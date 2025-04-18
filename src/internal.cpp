#include "internal.hpp"
#include <fstream>  // For file handling
#include <ctime>

namespace CaDiCaL {

/*------------------------------------------------------------------------*/
static Clause external_reason_clause;

Internal::Internal ()
    : mode (SEARCH), unsat (false), iterating (false),
      localsearching (false), lookingahead (false), preprocessing (false),
      protected_reasons (false), force_saved_phase (false),
      searching_lucky_phases (false), stable (false), reported (false),
      external_prop (false), did_external_prop (false),
      external_prop_is_lazy (true), forced_backt_allowed (false), 
      private_steps (false), rephased (0), vsize (0), max_var (0), 
      clause_id (0), original_id (0), reserved_ids (0), 
      conflict_id (0), concluded (false), lrat (false), level (0), vals (0),
      score_inc (1.0), scores (this), conflict (0), ignore (0),
      external_reason (&external_reason_clause), newest_clause (0),
      force_no_backtrack (false), from_propagator (false), ext_clause_forgettable (false),
      tainted_literal (0), notified (0), probe_reason (0), propagated (0),
      propagated2 (0), propergated (0), best_assigned (0),
      target_assigned (0), no_conflict_until (0), unsat_constraint (false),
      marked_failed (true), num_assigned (0), proof (0), lratbuilder (0),
      opts (this), in_assumptions (false), assumptions_num_iters (0), // amar
#ifndef QUIET
      profiles (this), force_phase_messages (false),
#endif
      arena (this), prefix ("c "), internal (this), external (0),
      termination_forced (false), vars (this->max_var),
      lits (this->max_var) {
  control.push_back (Level (0, 0));

  // The 'dummy_binary' is used in 'try_to_subsume_clause' to fake a real
  // clause, which then can be used to subsume or strengthen the given
  // clause in one routine for both binary and non binary clauses.  This
  // fake binary clause is always kept non-redundant (and not-moved etc.)
  // due to the following 'memset'.  Only literals will be changed.

  // In a previous version we used local automatic allocated 'Clause' on the
  // stack, which became incompatible with several compilers (see the
  // discussion on flexible array member in 'Clause.cpp').

  size_t bytes = Clause::bytes (2);
  dummy_binary = (Clause *) new char[bytes];
  memset (dummy_binary, 0, bytes);
  dummy_binary->size = 2;
}

Internal::~Internal () {
  delete[](char *) dummy_binary;
  for (const auto &c : clauses)
    delete_clause (c);
  if (proof)
    delete proof;
  if (lratbuilder)
    delete lratbuilder;
  for (auto &tracer : tracers)
    delete tracer;
  for (auto &filetracer : file_tracers)
    delete filetracer;
  for (auto &stattracer : stat_tracers)
    delete stattracer;
  if (vals) {
    vals -= vsize;
    delete[] vals;
  }
}

/*------------------------------------------------------------------------*/

// Values in 'vals' can be accessed in the range '[-max_var,max_var]' that
// is directly by a literal.  This is crucial for performance.  By shifting
// the start of 'vals' appropriately, we achieve that negative offsets from
// the start of 'vals' can be used.  We also need to set both values at
// 'lit' and '-lit' during assignments.  In MiniSAT integer literals are
// encoded, using the least significant bit as negation.  This avoids taking
// the 'abs ()' (as in our solution) and thus also avoids a branch in the
// hot-spot of the solver (clause traversal in propagation).  That solution
// requires another (branch less) negation of the values though and
// debugging is harder since literals occur only encoded in clauses.
// The main draw-back of our solution is that we have to shift the memory
// and access it through negative indices, which looks less clean (but still
// as far I can tell is properly defined C / C++).   You might get a warning
// by static analyzers though.  Clang with '--analyze' thought that this
// idiom would generate a memory leak thus we use the following dummy.

static signed char *ignore_clang_analyze_memory_leak_warning;

void Internal::enlarge_vals (size_t new_vsize) {
  signed char *new_vals;
  const size_t bytes = 2u * new_vsize;
  new_vals = new signed char[bytes]; // g++-4.8 does not like ... { 0 };
  memset (new_vals, 0, bytes);
  ignore_clang_analyze_memory_leak_warning = new_vals;
  new_vals += new_vsize;

  if (vals)
    memcpy (new_vals - max_var, vals - max_var, 2u * max_var + 1u);
  vals -= vsize;
  delete[] vals;
  vals = new_vals;
}

/*------------------------------------------------------------------------*/

template <class T>
static void enlarge_init (vector<T> &v, size_t N, const T &i) {
  if (v.size () < N)
    v.resize (N, i);
}

template <class T> static void enlarge_only (vector<T> &v, size_t N) {
  if (v.size () < N)
    v.resize (N, T ());
}

template <class T> static void enlarge_zero (vector<T> &v, size_t N) {
  enlarge_init (v, N, (const T &) 0);
}

/*------------------------------------------------------------------------*/

void Internal::enlarge (int new_max_var) {
  // New variables can be created that can invoke enlarge anytime (via calls
  // during ipasir-up call-backs), thus assuming (!level) is not correct 
  size_t new_vsize = vsize ? 2 * vsize : 1 + (size_t) new_max_var;
  while (new_vsize <= (size_t) new_max_var)
    new_vsize *= 2;
  LOG ("enlarge internal size from %zd to new size %zd", vsize, new_vsize);
  // Ordered in the size of allocated memory (larger block first).
  enlarge_zero (unit_clauses, 2 * new_vsize);
  enlarge_only (wtab, 2 * new_vsize);
  enlarge_only (vtab, new_vsize);
  enlarge_zero (parents, new_vsize);
  enlarge_only (links, new_vsize);
  enlarge_zero (btab, new_vsize);
  enlarge_zero (gtab, new_vsize);
  enlarge_zero (stab, new_vsize);
  enlarge_init (ptab, 2 * new_vsize, -1);
  enlarge_only (ftab, new_vsize);
  enlarge_vals (new_vsize);
  enlarge_zero (frozentab, new_vsize);
  enlarge_zero (relevanttab, new_vsize);
  const signed char val = opts.phase ? 1 : -1;
  enlarge_init (phases.saved, new_vsize, val);
  enlarge_zero (phases.forced, new_vsize);
  enlarge_zero (phases.target, new_vsize);
  enlarge_zero (phases.best, new_vsize);
  enlarge_zero (phases.prev, new_vsize);
  enlarge_zero (phases.min, new_vsize);
  enlarge_zero (marks, new_vsize);
  vsize = new_vsize;
}

void Internal::init_vars (int new_max_var) {
  if (new_max_var <= max_var)
    return;
  // New variables can be created that can invoke enlarge anytime (via calls
  // during ipasir-up call-backs), thus assuming (!level) is not correct 
  LOG ("initializing %d internal variables from %d to %d",
       new_max_var - max_var, max_var + 1, new_max_var);
  if ((size_t) new_max_var >= vsize)
    enlarge (new_max_var);
#ifndef NDEBUG
  for (int64_t i = -new_max_var; i < -max_var; i++)
    assert (!vals[i]);
  for (unsigned i = max_var + 1; i <= (unsigned) new_max_var; i++)
    assert (!vals[i]), assert (!btab[i]), assert (!gtab[i]);
  for (uint64_t i = 2 * ((uint64_t) max_var + 1);
       i <= 2 * (uint64_t) new_max_var + 1; i++)
    assert (ptab[i] == -1);
#endif
  assert (!btab[0]);
  int old_max_var = max_var;
  max_var = new_max_var;
  init_queue (old_max_var, new_max_var);
  init_scores (old_max_var, new_max_var);
  int initialized = new_max_var - old_max_var;
  stats.vars += initialized;
  stats.unused += initialized;
  stats.inactive += initialized;
  LOG ("finished initializing %d internal variables", initialized);
}

void Internal::add_original_lit (int lit) {
  assert (abs (lit) <= max_var);
  if (lit) {
    original.push_back (lit);
  } else {
    const uint64_t id =
        original_id < reserved_ids ? ++original_id : ++clause_id;
    if (proof) {
      // Use the external form of the clause for printing in proof
      // Externalize(internalized literal) != external literal
      assert (!original.size () || !external->eclause.empty ());
      proof->add_external_original_clause (id, false, external->eclause);
    }
    if (internal->opts.check &&
      (internal->opts.checkwitness || internal->opts.checkfailed)) {
      bool forgettable = from_propagator && ext_clause_forgettable;
      if (forgettable) {
        assert (!original.size () || !external->eclause.empty ());

        // First integer is the presence-flag (even if the clause is empty)
        external->forgettable_original[id] = {1};
       
        for (auto const& elit : external->eclause)
          external->forgettable_original[id].push_back(elit);
        
        LOG (external->eclause, "clause added to external forgettable map:");
      }
    }
    
    add_new_original_clause (id);
    original.clear ();
  }
}

void Internal::finish_added_clause_with_id (uint64_t id, bool restore) {
  if (proof) {
    // Use the external form of the clause for printing in proof
    // Externalize(internalized literal) != external literal
    assert (!original.size () || !external->eclause.empty ());
    proof->add_external_original_clause (id, false, external->eclause,
                                         restore);
  }
  add_new_original_clause (id);
  original.clear ();
}

/*------------------------------------------------------------------------*/

void Internal::reserve_ids (int number) {
  // return;
  LOG ("reserving %d ids", number);
  assert (number >= 0);
  assert (!clause_id && !reserved_ids && !original_id);
  clause_id = reserved_ids = number;
  if (proof)
    proof->begin_proof (reserved_ids);
}

//amar
void Internal::set_assumptions_mode (bool is_assumption) {
  in_assumptions = is_assumption;
}

// void Internal::set_assumptions (vector<int> assumptions) {
//   assumptions = assumptions;
// }



/*------------------------------------------------------------------------*/

// This is the main CDCL loop with interleaved inprocessing.

int Internal::cdcl_loop_with_inprocessing () {

  int res = 0;

  START (search);

  if (stable) {
    START (stable);
    report ('[');
  } else {
    START (unstable);
    report ('{');
  }

  std::ofstream outFile;
  char* filename = getenv("CADICAL_FILENAME");
  std::string filename_str = filename;  // Implicit conversion
  filename_str += "_inprocessing";
  outFile.open (filename_str);
  if (!outFile) {
      error ("Error: File could not be created.");
  }

  std::ofstream outFile_pr;
  std::string filename_pr = filename_str;  // Implicit conversion
  filename_pr += "_pr";

  outFile_pr.open (filename_pr);
  if (!outFile_pr) {
      error ("Error: File could not be created.");
  }



  while (!res) {
    // assumptions_num_iters += 1;
    // if (in_assumptions && assumptions_num_iters > 1000) {
    //   printf("exiting solving mode \n");
    //   res = 20;
    // }
    // print_assignment ();
    if (unsat) {
      LOG ("pre: we are in the unsat case");
      if (in_assumptions) {
        LOG ("we are in the unsat case");
        // reduce (); // collect useless clauses
      }
      res = 20;
    } else if (unsat_constraint) {
      res = 20;
    } else if (!propagate ()) {
      LOG("conflict found in CDCL : propagate");
      LOG(trail, "in CDCL the trail is:");
      analyze (); // propagate and analyze
    } else if (iterating) {
      LOG("conflict found in CDCL : iterate");
      iterate ();                               // report learned unit
    } else if (!external_propagate () || unsat) { // external propagation
      LOG("conflict found in CDCL : external propagate");
      if (unsat) {
        continue;
      } else
        analyze ();
    } else if (satisfied ()) { // found model
      if (!external_check_solution () || unsat) {
        if (unsat)
          continue;
        else
          analyze ();
      } else if (satisfied ())
        res = 10;
    } else if (search_limits_hit ()) {
      break;                               // decision or conflict limit
    } else if (terminated_asynchronously ()) // externally terminated 
      break;
    else if (restarting ()) {
      // printf ("we are restarting from level: %d", level);
      restart (); // restart by backtracking
    } else if (rephasing ())
      rephase (); // reset variable phases
    else if (reducing ())
      reduce (); // collect useless clauses
    else if (probing ())
      probe (); // failed literal probing
    else if (subsuming ())
      subsume (); // subsumption algorithm
    else if (eliminating ())
      elim (); // variable elimination
    else if (compacting ())
      compact (); // collect variables
    else if (conditioning ())
      condition (); 

      
    // else if (rat_finding ()) {
    //   if (detect_rat ()) {
    //     clause = assumptions;
    //     // new_learned_redundant_clause (1);
    //     clause.clear ();
    //     // Clause* assumptions_clause = assumptions;
    //     // new_clause_as (assumptions);
    //     learn_unit_clause (-assumptions[0]);
    //     reduce ();
    //   }
    //   res = 20;
    // }
    else if (directioning ()) {
      if (direction ()) {
        LOG("we are reducing (at direction)");
        reduce ();
      }
      res = 20;
    }
    // changed this to globalling decide
    // else if (globalling ()) {
    //   bool added_a_clause = least_conditional_part(outFile, outFile_pr);

    //   // unecessary because we return false if we find a conflict through propagation
    //   // if (unsat) {
    //   //   continue;
    //   // }

    //   // remake the same old decision -> this is super hacky
    //   // I currently need this because the solver expects us to be at a certain decsion level
    //   // todo: find a better way to do this
    //   if (added_a_clause && !opts.globalbcp) {
    //     backtrack ();
    //     // print_assignment ();
    //     // if (!propagate ()) {
    //     //   printf("found a conflict here!");
    //     //   analyze ();
    //     // }
    //     printf("propagated once at level %d! \n", level);
    //     // decide ();
    //     Flags &f = flags (global_decision1);
    //     bool second_propagate = true;
    //     if (!(f.status == Flags::FIXED)) {
    //       printf("Literal %d is not fixed \n", global_decision1);
    //       search_assume_decision (global_decision1);
    //       // printf("At internal position 1; propagated: %d; trail.size: %d \n", propagated, trail.size ());
    //       if (!propagate ()) {
    //         // printf("At internal position 2; propagated: %d; trail.size: %d \n", propagated, trail.size ());

    //         printf("found a conflict here!");
    //         analyze ();
    //         // printf("At internal position 3; propagated: %d; trail.size: %d \n", propagated, trail.size ());
    //         if (!propagate ()) {
    //           // printf("At internal position 4; propagated: %d; trail.size: %d \n", propagated, trail.size ());
    //           printf("Found a double conflict on first propagate!");
    //           second_propagate = false;
    //         }
    //       }
    //     }
    //     // printf("At internal position 5; propagated: %d; trail.size: %d \n", propagated, trail.size ());

    //     printf("propagated twice at level %d! \n", level);
    //     // decide ();
    //     // kinda hacky ~but I was running into a problem with pigeonhole where global_decision2 was 0
    //     if (global_decision2 && second_propagate) {
    //       Flags &g = flags (global_decision2);
    //       if (!(g.status == Flags::FIXED) && !val (global_decision2)) {
    //         printf("Literal %d is not fixed \n", global_decision2);
    //         search_assume_decision (global_decision2);
    //           // printf("At internal position 6; propagated: %d; trail.size: %d \n", propagated, trail.size ());
    //         if (!propagate ()) {
    //           printf("found a conflict here!");
    //           analyze ();
    //            if (!propagate ()) {
    //               printf("Found a double conflict on second propagate!");
    //               analyze ();
    //               // todo: might need to propagate after this
    //             }
    //         }
    //       }
    //     }
    //     printf("propagated thrice at level %d! \n", level);
    //     // todo: not sure if this will actually do what I want it to do
    //     reset_assumptions ();
    //   }


    //   if (in_assumptions) { // && added_a_clause) {
    //     // trying to reduce after each round of assumptions
    //     // seems to make things slightly faster:
    //     // can do pigeonhole 40 in  134.11 seconds
    //     // reduce (); // collect useless clauses
    //     res = 20;
    //   } else {
    //     // printf("we are NOT in assumptions! \n");
    //   }
    // } 
    else
      res = decide (); // next decision
  }

  // adding a reduce step here
  if (level == 0 && in_assumptions) {
    LOG("making a reduction here with level: %D", level);
    reduce ();
  }


  

  if (stable) {
    STOP (stable);
    report (']');
  } else {
    STOP (unstable);
    report ('}');
  }

  STOP (search);

  return res;
}

/*------------------------------------------------------------------------*/

// Most of the limits are only initialized in the first 'solve' call and
// increased as in a stand-alone non-incremental SAT call except for those
// explicitly marked as being reset below.

void Internal::init_report_limits () {
  reported = false;
  lim.report = 0;
}

void Internal::init_preprocessing_limits () {

  const bool incremental = lim.initialized;
  if (incremental)
    LOG ("reinitializing preprocessing limits incrementally");
  else
    LOG ("initializing preprocessing limits and increments");

  const char *mode = 0;

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.subsume = stats.conflicts + scale (opts.subsumeint);
    mode = "initial";
  }
  (void) mode;
  LOG ("%s subsume limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.subsume, lim.subsume - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    last.elim.marked = -1;
    lim.elim = stats.conflicts + scale (opts.elimint);
    mode = "initial";
  }
  (void) mode;
  LOG ("%s elim limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.elim, lim.elim - stats.conflicts);

  // Initialize and reset elimination bounds in any case.

  lim.elimbound = opts.elimboundmin;
  LOG ("elimination bound %" PRId64 "", lim.elimbound);

  /*----------------------------------------------------------------------*/

  if (!incremental) {

    last.ternary.marked = -1; // TODO explain why this is necessary.

    lim.compact = stats.conflicts + opts.compactint;
    LOG ("initial compact limit %" PRId64 " increment %" PRId64 "",
         lim.compact, lim.compact - stats.conflicts);
  }

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.probe = stats.conflicts + opts.probeint;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s probe limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.probe, lim.probe - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.condition = stats.conflicts + opts.conditionint;
    mode = "initial";
  }
  LOG ("%s condition limit %" PRId64 " increment %" PRId64, mode,
       lim.condition, lim.condition - stats.conflicts);

  /*----------------------------------------------------------------------*/

  // Initial preprocessing rounds.

  if (inc.preprocessing <= 0) {
    lim.preprocessing = 0;
    LOG ("no preprocessing");
  } else {
    lim.preprocessing = inc.preprocessing;
    LOG ("limiting to %" PRId64 " preprocessing rounds", lim.preprocessing);
  }
}

void Internal::init_search_limits () {

  const bool incremental = lim.initialized;
  if (incremental)
    LOG ("reinitializing search limits incrementally");
  else
    LOG ("initializing search limits and increments");

  const char *mode = 0;

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    last.reduce.conflicts = -1;
    lim.reduce = stats.conflicts + opts.reduceint;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s reduce limit %" PRId64 " after %" PRId64 " conflicts", mode,
       lim.reduce, lim.reduce - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (incremental)
    mode = "keeping";
  else {
    lim.flush = opts.flushint;
    inc.flush = opts.flushint;
    mode = "initial";
  }
  (void) mode;
  LOG ("%s flush limit %" PRId64 " interval %" PRId64 "", mode, lim.flush,
       inc.flush);

  /*----------------------------------------------------------------------*/

  // Initialize or reset 'rephase' limits in any case.

  lim.rephase = stats.conflicts + opts.rephaseint;
  lim.rephased[0] = lim.rephased[1] = 0;
  LOG ("new rephase limit %" PRId64 " after %" PRId64 " conflicts",
       lim.rephase, lim.rephase - stats.conflicts);

  /*----------------------------------------------------------------------*/

  // Initialize or reset 'restart' limits in any case.

  lim.restart = stats.conflicts + opts.restartint;
  LOG ("new restart limit %" PRId64 " increment %" PRId64 "", lim.restart,
       lim.restart - stats.conflicts);

  /*----------------------------------------------------------------------*/

  if (!incremental) {
    stable = opts.stabilize && opts.stabilizeonly;
    if (stable)
      LOG ("starting in always forced stable phase");
    else
      LOG ("starting in default non-stable phase");
    init_averages ();
  } else if (opts.stabilize && opts.stabilizeonly) {
    LOG ("keeping always forced stable phase");
    assert (stable);
  } else if (stable) {
    LOG ("switching back to default non-stable phase");
    stable = false;
    swap_averages ();
  } else
    LOG ("keeping non-stable phase");

  inc.stabilize = opts.stabilizeint;
  lim.stabilize = stats.conflicts + inc.stabilize;
  LOG ("new stabilize limit %" PRId64 " after %" PRId64 " conflicts",
       lim.stabilize, inc.stabilize);

  if (opts.stabilize && opts.reluctant) {
    LOG ("new restart reluctant doubling sequence period %d",
         opts.reluctant);
    reluctant.enable (opts.reluctant, opts.reluctantmax);
  } else
    reluctant.disable ();

  /*----------------------------------------------------------------------*/

  // Conflict and decision limits.

  if (inc.conflicts < 0) {
    lim.conflicts = -1;
    LOG ("no limit on conflicts");
  } else {
    lim.conflicts = stats.conflicts + inc.conflicts;
    LOG ("conflict limit after %" PRId64 " conflicts at %" PRId64
         " conflicts",
         inc.conflicts, lim.conflicts);
  }

  if (inc.decisions < 0) {
    lim.decisions = -1;
    LOG ("no limit on decisions");
  } else {
    lim.decisions = stats.decisions + inc.decisions;
    LOG ("conflict limit after %" PRId64 " decisions at %" PRId64
         " decisions",
         inc.decisions, lim.decisions);
  }

  /*----------------------------------------------------------------------*/

  // Initial preprocessing rounds.

  if (inc.localsearch <= 0) {
    lim.localsearch = 0;
    LOG ("no local search");
  } else {
    lim.localsearch = inc.localsearch;
    LOG ("limiting to %" PRId64 " local search rounds", lim.localsearch);
  }

  /*----------------------------------------------------------------------*/

  lim.initialized = true;
}

/*------------------------------------------------------------------------*/

bool Internal::preprocess_round (int round) {
  (void) round;
  if (unsat)
    return false;
  if (!max_var)
    return false;
  START (preprocess);
  struct {
    int64_t vars, clauses;
  } before, after;
  before.vars = active ();
  before.clauses = stats.current.irredundant;
  stats.preprocessings++;
  assert (!preprocessing);
  preprocessing = true;
  PHASE ("preprocessing", stats.preprocessings,
         "starting round %d with %" PRId64 " variables and %" PRId64
         " clauses",
         round, before.vars, before.clauses);
  int old_elimbound = lim.elimbound;
  if (opts.probe)
    probe (false);
  if (opts.elim)
    elim (false);
  if (opts.condition)
    condition (false);
  after.vars = active ();
  after.clauses = stats.current.irredundant;
  assert (preprocessing);
  preprocessing = false;
  PHASE ("preprocessing", stats.preprocessings,
         "finished round %d with %" PRId64 " variables and %" PRId64
         " clauses",
         round, after.vars, after.clauses);
  STOP (preprocess);
  report ('P');
  if (unsat)
    return false;
  if (after.vars < before.vars)
    return true;
  if (old_elimbound < lim.elimbound)
    return true;
  return false;
}

int Internal::preprocess () {
  for (int i = 0; i < lim.preprocessing; i++)
    if (!preprocess_round (i))
      break;
  if (unsat)
    return 20;
  return 0;
}

// void Internal::propagate_for_unit(int lit) {
//     backtrack ();
//     printf("We are propagating on just %d \n", lit);
//     printf("We have propagated: %d and trail.size %d \n", propagated, trail.size ());
//     Flags &f = Internal::flags (lit);
//     if (f.status == Flags::FIXED || !val (lit)) {
//       printf("skipping ALL of %d \n", lit);
//       return;
//     }

//     search_assume_decision (lit);
//     if (!propagate ()) {
//         printf ("We reach a conflict from a single propagation on %d and will analyze\n", lit);
//         analyze ();
//         if (!propagate ()) {
//           analyze ();
//           printf ("We should have reached a contradiction! \n");
//           // if (!propagate ()) {
//           //   printf("Made it inside the unecessary propagation! \n");
//           // }
//         }
//     } 
//     // else {
//     //   bool added_a_clause = least_conditional_part(outFile, outFile_pr);
//     // }
// }

// gets all of the touched literals based on the current assignment
// this heuristic is described in PreLearn paper
vector<int> Internal::get_touched_literals () {
  vector<int> touched_literals;
  for (auto &c : clauses) {
    bool clause_touched = false;
    bool clause_satisfied = false;
    vector<int> variables_to_consider;
    for (auto l : *c) {
      if (val (l) > 0) {
        clause_satisfied = true;
        break;
      } else if (val (l) < 0) {
        clause_touched = true;
      } else if (!getbit (l, 1) && !(Internal::flags (l).status == Flags::FIXED)) {
          variables_to_consider.push_back(l);
      }
    }
    if (clause_touched && !clause_satisfied) {
      for (auto l : variables_to_consider) {
          touched_literals.push_back(l);
          setbit (l, 1);
      }
    }
  }
  for (auto l : touched_literals) {
    unsetbit (l, 1);
  }

  return touched_literals;
}

// amar : created an option to learn globally blocked clauses in a preprocessing step
int Internal::global_preprocess () {
   std::ofstream outFile;
  char* filename = getenv("CADICAL_FILENAME");
  outFile.open (filename);
  if (!outFile) {
      error ("Error: File could not be created.");
  }

  std::ofstream outFile_pr;
  std::string filename_pr = filename;  // Implicit conversion
  filename_pr += "_pr";
  unsigned int seed = static_cast<unsigned int>(std::time(NULL)); // Store the seed
  // unsigned int seed = 1744666512;
  printf("We are using the SEED: %d\n", seed);
  srand(seed);

  outFile_pr.open (filename_pr);
  if (!outFile_pr) {
      error ("Error: File could not be created.");
  }

  for (int count = 1; count <= Internal::max_var; count++) {
    int i_no_polarity = (rand() % max_var) + 1;
    int i_polarity = (rand() % 2);
    // int i = (i_polarity ? -1 : 1) * i_no_polarity;
    int i = i_no_polarity;
    // i = 413;
    // i = count;
    backtrack ();
    // need to have this outside to skip the extra unnecessary loops
    Flags &f = Internal::flags (i);
    if (f.status == Flags::FIXED) {
      printf("skipping ALL of %d \n", i);
      continue;
    }
    printf("deciding on i: %d\n", i);
    search_assume_decision (i);
    if (!propagate ()) {
      printf("1.Right before analyze!\n");
      analyze ();
      if (!propagate ()) {
        printf("IN EARLY PROPAGATE: found unsat from %d\n", i);
        break;
      }
      printf("IN EARLY PROPAGATE: found conflict from %d\n", i);
      continue;
    }
    vector<int> touched_literals = get_touched_literals ();
    printf("For literal %d, we touch: ", i);
    print_vector (touched_literals);
    // for (int j = i + 1; j <= Internal::max_var; j++) {
    for (auto j : touched_literals) {
        assert (!unsat);
        for (int polarity : {-1, 1}) { 
          assert (!unsat);
          int j_polar = polarity * j;
          // j_polar = 428;
          // if (i > 15) 
          //   j_polar = i - 14;
          // else
          //   j_polar = i + 14;
          printf("We are propagating on %d and %d \n", i, j_polar);
          printf("We have propagated: %d and trail.size %d \n", propagated, trail.size ());

          // need to have this in the inner loop beause this could be set in the inner loop!!!!
          Flags &f = Internal::flags (i);
          if (f.status == Flags::FIXED) {
            printf("skipping ALL of %d \n", i);
            break;
          }
          Flags &f2 = Internal::flags (j_polar);
          if (f2.status == Flags::FIXED) {
            printf("skipping one interation of %d \n", j_polar);
            continue;
          }
          backtrack ();
          search_assume_decision (i);
          if (!propagate ()) {
            printf ("We reach a conflict from a single propagation on %d and will analyze\n", i);
            printf("1.Right before analyze!\n");
            analyze ();
            if (!propagate ()) {
              printf("2.Right before analyze!\n");
              analyze ();
              printf ("We should have reached a contradiction! \n");
              break;
              // if (!propagate ()) {
              //   printf("Made it inside the unecessary propagation! \n");
              // }
            }
          } else {
            if (val (j_polar) != 0) {
              printf("propagating %d give %d \n", i, j_polar);
              continue;
            }
            search_assume_decision (j_polar);
            if (!propagate ()) {
              printf("3.Right before analyze!\n");
              analyze ();
              if (!unsat && !propagate ()) {
                printf("4.Right before analyze!\n");
                analyze ();
                if (!unsat && !propagate ()) {
                  printf("5.Right before analyze!\n");
                  analyze ();
                }
              }
            } else {
              printf("Before added_a_clause, we have propagated: %d and trail.size %d \n", propagated, trail.size ());
              bool added_a_clause = least_conditional_part(outFile, outFile_pr);
              printf("After added_a_clause, we have propagated: %d and trail.size %d \n", propagated, trail.size ());
              backtrack ();
              printf("We have just finished adding a clause step and returned %d!\n", added_a_clause);
            }
          }
        if (unsat) break;
      }
      if (unsat) break;
    }
    printf("out of loop!\n");
    if (unsat) break;
    backtrack ();
    f = Internal::flags (i);
    if (f.status == Flags::FIXED) {
      printf("skipping ALL of %d \n", i);
      continue;
    }
    search_assume_decision (i);
    if (!propagate ()) {
      printf("6.Right before analyze!\n");
      analyze ();
      backtrack ();
      continue;
    }
    backtrack ();
    assert (Internal::flags (i).status != Flags::FIXED);
    search_assume_decision (-i);
    if (!propagate ())
      analyze ();
    if (unsat) break;
  }


}

/*------------------------------------------------------------------------*/

int Internal::try_to_satisfy_formula_by_saved_phases () {
  LOG ("satisfying formula by saved phases");
  assert (!level);
  assert (!force_saved_phase);
  assert (propagated == trail.size ());
  force_saved_phase = true;
  if (external_prop) {
    assert (!level);
    LOG ("external notifications are turned off during preprocessing.");
    private_steps = true;
  }
  int res = 0;
  while (!res) {
    if (satisfied ()) {
      LOG ("formula indeed satisfied by saved phases");
      res = 10;
    } else if (decide ()) {
      LOG ("inconsistent assumptions with redundant clauses and phases");
      res = 20;
    } else if (!propagate ()) {
      LOG ("saved phases do not satisfy redundant clauses");
      assert (level > 0);
      backtrack ();
      conflict = 0; // ignore conflict
      assert (!res);
      break;
    }
  }
  assert (force_saved_phase);
  force_saved_phase = false;
  if (external_prop) {
    private_steps = false;
    LOG("external notifications are turned back on.");
    if (!level) notify_assignments (); // In case fixed assignments were found.
    else {
      renotify_trail_after_local_search ();
    }
  }
  return res;
}

/*------------------------------------------------------------------------*/

void Internal::produce_failed_assumptions () {
  LOG ("producing failed assumptions");
  assert (!level);
  assert (!assumptions.empty ());
  while (!unsat) {
    assert (!satisfied ());
    notify_assignments ();
    if (decide ())
      break;
    while (!unsat && !propagate ())
      analyze ();
  }
  notify_assignments ();
  if (unsat)
    LOG ("formula is actually unsatisfiable unconditionally");
  else
    LOG ("assumptions indeed failing");
}

/*------------------------------------------------------------------------*/

int Internal::local_search_round (int round) {

  assert (round > 0);

  if (unsat)
    return false;
  if (!max_var)
    return false;

  START_OUTER_WALK ();
  assert (!localsearching);
  localsearching = true;

  // Determine propagation limit quadratically scaled with rounds.
  //
  int64_t limit = opts.walkmineff;
  limit *= round;
  if (LONG_MAX / round > limit)
    limit *= round;
  else
    limit = LONG_MAX;

  int res = walk_round (limit, true);

  assert (localsearching);
  localsearching = false;
  STOP_OUTER_WALK ();

  report ('L');

  return res;
}

int Internal::local_search () {

  if (unsat)
    return 0;
  if (!max_var)
    return 0;
  if (!opts.walk)
    return 0;
  if (constraint.size ())
    return 0;

  int res = 0;

  for (int i = 1; !res && i <= lim.localsearch; i++)
    res = local_search_round (i);

  if (res == 10) {
    LOG ("local search determined formula to be satisfiable");
    assert (!stats.walk.minimum);
    res = try_to_satisfy_formula_by_saved_phases ();
  } else if (res == 20) {
    LOG ("local search determined assumptions to be inconsistent");
    assert (!assumptions.empty ());
    produce_failed_assumptions ();
  }

  return res;
}

/*------------------------------------------------------------------------*/

// if preprocess_only is false and opts.ilb is true we do not preprocess
// such that we do not have to backtrack to level 0.
// TODO: check restore_clauses works on higher level
//
int Internal::solve (bool preprocess_only) {
  assert (clause.empty ());
  START (solve);
  if (proof)
    proof->solve_query ();
  if (opts.ilb) {
    if (opts.ilbassumptions)
      sort_and_reuse_assumptions ();
    stats.ilbtriggers++;
    stats.ilbsuccess += (level > 0);
    stats.levelsreused += level;
    if (level) {
      assert (control.size () > 1);
      stats.literalsreused += num_assigned - control[1].trail;
    }
    if (external->propagator) 
      renotify_trail_after_ilb ();
  }
  if (preprocess_only)
    LOG ("internal solving in preprocessing only mode");
  else
    LOG ("internal solving in full mode");
  init_report_limits ();
  int res = already_solved ();
  if (!res && preprocess_only && level)
    backtrack ();
  if (!res)
    res = restore_clauses ();
  if (!res) {
    init_preprocessing_limits ();
    if (!preprocess_only)
      init_search_limits ();
  }
  if (!res && !level)
    res = preprocess ();
  if (!res && opts.globalpreprocess)
    res = global_preprocess ();
  if (!preprocess_only) {
    if (!res && !level)
      res = local_search ();
    if (!res && !level)
      res = lucky_phases ();
    if (!res || (res == 10 && external_prop)) {
      if (res == 10 && external_prop && level)
        backtrack ();
      res = cdcl_loop_with_inprocessing ();
    }
  }
  finalize (res);
  reset_solving ();
  report_solving (res);
  STOP (solve);
  std::ofstream outFile_conflicts;
  char* filename_original = getenv("CADICAL_FILENAME");
  if (filename_original) {
      std::string filename_conflicts = filename_original;  // Implicit conversion
      filename_conflicts += "_conflicts";

      std::ofstream outFile_conflicts;
      outFile_conflicts.open (filename_conflicts);
      if (!outFile_conflicts) {
          error ("Error: File could not be created.");
      }
      outFile_conflicts << stats.conflicts;
  }
  return res;
}

int Internal::already_solved () {
  int res = 0;
  if (unsat || unsat_constraint) {
    LOG ("already inconsistent");
    res = 20;
  } else {
    if (level && !opts.ilb)
      backtrack ();
    if (!level && !propagate ()) {
      LOG ("root level propagation produces conflict");
      learn_empty_clause ();
      res = 20;
    }
    if (max_var == 0 && res == 0)
      res = 10;
  }
  return res;
}
void Internal::report_solving (int res) {
  if (res == 10)
    report ('1');
  else if (res == 20)
    report ('0');
  else
    report ('?');
}

void Internal::reset_solving () {
  if (termination_forced) {

    // TODO this leads potentially to a data race if the external
    // user is calling 'terminate' twice within one 'solve' call.
    // A proper solution would be to guard / protect setting the
    // 'termination_forced' flag and only allow it during solving and
    // ignore it otherwise thus also the second time it is called during a
    // 'solve' call.  We could move resetting it also the start of
    // 'solve'.
    //
    termination_forced = false;

    LOG ("reset forced termination");
  }
}

int Internal::restore_clauses () {
  int res = 0;
  if (opts.restoreall <= 1 && external->tainted.empty ()) {
    LOG ("no tainted literals and nothing to restore");
    report ('*');
  } else {
    report ('+');
    remove_garbage_binaries ();
    external->restore_clauses ();
    internal->report ('r');
    if (!unsat && !level && !propagate ()) {
      LOG ("root level propagation after restore produces conflict");
      learn_empty_clause ();
      res = 20;
    }
  }
  return res;
}

int Internal::lookahead () {
  assert (clause.empty ());
  START (lookahead);
  assert (!lookingahead);
  lookingahead = true;
  if (external_prop) {
    if (level) {
      // Combining lookahead with external propagator is limited
      // Note that lookahead_probing (); would also force backtrack anyway
      backtrack ();
    }
    LOG ("external notifications are turned off during preprocessing.");
    private_steps = true;
  }
  int tmp = already_solved ();
  if (!tmp)
    tmp = restore_clauses ();
  int res = 0;
  if (!tmp)
    res = lookahead_probing ();
  if (res == INT_MIN)
    res = 0;
  reset_solving ();
  report_solving (tmp);
  assert (lookingahead);
  lookingahead = false;
  STOP (lookahead);
  if (external_prop) {
    private_steps = false;
    LOG("external notifications are turned back on.");
    notify_assignments (); // In case fixed assignments were found.
  }
  return res;
}

/*------------------------------------------------------------------------*/

void Internal::finalize (int res) {
  if (!proof)
    return;
  LOG ("finalizing");
  // finalize external units
  for (const auto &evar : external->vars) {
    assert (evar > 0);
    const auto eidx = 2 * evar;
    int sign = 1;
    uint64_t id = external->ext_units[eidx];
    if (!id) {
      sign = -1;
      id = external->ext_units[eidx + 1];
    }
    if (id) {
      proof->finalize_external_unit (id, evar * sign);
    }
  }
  // finalize internal units
  for (const auto &lit : lits) {
    const auto elit = externalize (lit);
    if (elit) {
      const unsigned eidx = (elit < 0) + 2u * (unsigned) abs (elit);
      const uint64_t id = external->ext_units[eidx];
      if (id) {
        assert (unit_clauses[vlit (lit)] == id);
        continue;
      }
    }
    const auto uidx = vlit (lit);
    const uint64_t id = unit_clauses[uidx];
    if (!id)
      continue;
    proof->finalize_unit (id, lit);
  }
  // See the discussion in 'propagate' on why garbage binary clauses stick
  // around.
  for (const auto &c : clauses)
    if (!c->garbage || c->size == 2)
      proof->finalize_clause (c);

  // finalize conflict and proof
  if (conflict_id) {
    proof->finalize_clause (conflict_id, {});
  }
  proof->report_status (res, conflict_id);
  if (res == 10)
    external->conclude_sat ();
  else if (res == 20)
    conclude_unsat ();
}

/*------------------------------------------------------------------------*/

void Internal::print_statistics () {
  stats.print (this);
  for (auto &st : stat_tracers)
    st->print_stats ();
}

/*------------------------------------------------------------------------*/

// Only useful for debugging purposes.

void Internal::dump (Clause *c) {
  for (const auto &lit : *c)
    printf ("%d ", lit);
  printf ("0\n");
}

void Internal::dump () {
  int64_t m = assumptions.size ();
  for (auto idx : vars)
    if (fixed (idx))
      m++;
  for (const auto &c : clauses)
    if (!c->garbage)
      m++;
  printf ("p cnf %d %" PRId64 "\n", max_var, m);
  for (auto idx : vars) {
    const int tmp = fixed (idx);
    if (tmp)
      printf ("%d 0\n", tmp < 0 ? -idx : idx);
  }
  for (const auto &c : clauses)
    if (!c->garbage)
      dump (c);
  for (const auto &lit : assumptions)
    printf ("%d 0\n", lit);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/

bool Internal::traverse_constraint (ClauseIterator &it) {
  if (constraint.empty () && !unsat_constraint)
    return true;

  vector<int> eclause;
  if (unsat)
    return it.clause (eclause);

  LOG (constraint, "traversing constraint");
  bool satisfied = false;
  for (auto ilit : constraint) {
    const int tmp = fixed (ilit);
    if (tmp > 0) {
      satisfied = true;
      break;
    }
    if (tmp < 0)
      continue;
    const int elit = externalize (ilit);
    eclause.push_back (elit);
  }
  if (!satisfied && !it.clause (eclause))
    return false;

  return true;
}
/*------------------------------------------------------------------------*/

bool Internal::traverse_clauses (ClauseIterator &it) {
  vector<int> eclause;
  if (unsat)
    return it.clause (eclause);
  for (const auto &c : clauses) {
    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    bool satisfied = false;
    for (const auto &ilit : *c) {
      const int tmp = fixed (ilit);
      if (tmp > 0) {
        satisfied = true;
        break;
      }
      if (tmp < 0)
        continue;
      const int elit = externalize (ilit);
      eclause.push_back (elit);
    }
    if (!satisfied && !it.clause (eclause))
      return false;
    eclause.clear ();
  }
  return true;
}

} // namespace CaDiCaL
