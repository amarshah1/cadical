#include "internal.hpp"

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

inline unsigned LratChecker::l2u (int lit) {
  assert (lit);
  assert (lit != INT_MIN);
  unsigned res = 2 * (abs (lit) - 1);
  if (lit < 0)
    res++;
  return res;
}

signed char &LratChecker::mark (int lit) {
  const unsigned u = l2u (lit);
  assert (u < marks.size ());
  return marks[u];
}

signed char &LratChecker::checked_lit (int lit) {
  const unsigned u = l2u (lit);
  assert (u < checked_lits.size ());
  return checked_lits[u];
}

/*------------------------------------------------------------------------*/

LratCheckerClause *LratChecker::new_clause () {
  const size_t size = imported_clause.size ();
  assert (size <= UINT_MAX);
  const int off = size ? 1 : 0;
  const size_t bytes =
      sizeof (LratCheckerClause) + (size - off) * sizeof (int);
  LratCheckerClause *res = (LratCheckerClause *) new char[bytes];
  res->garbage = false;
  res->next = 0;
  res->hash = last_hash;
  res->id = last_id;
  res->size = size;
  res->used = false;
  res->tautological = false;
  int *literals = res->literals, *p = literals;
  for (auto &b : checked_lits)
    b = false;
  for (const auto &lit : imported_clause) {
    *p++ = lit;
    checked_lit (-lit) = true;
    if (checked_lit (lit)) {
      LOG (imported_clause, "LRAT CHECKER clause tautological");
      res->tautological = true;
    }
  }
  for (const auto &lit : imported_clause)
    checked_lit (-lit) = false;
  num_clauses++;

  return res;
}

void LratChecker::delete_clause (LratCheckerClause *c) {
  assert (c);
  if (!c->garbage) {
    assert (num_clauses);
    num_clauses--;
  } else {
    assert (num_garbage);
    num_garbage--;
  }
  delete[](char *) c;
}

void LratChecker::enlarge_clauses () {
  assert (num_clauses == size_clauses);
  const uint64_t new_size_clauses = size_clauses ? 2 * size_clauses : 1;
  LOG ("LRAT CHECKER enlarging clauses of checker from %" PRIu64
       " to %" PRIu64,
       (uint64_t) size_clauses, (uint64_t) new_size_clauses);
  LratCheckerClause **new_clauses;
  new_clauses = new LratCheckerClause *[new_size_clauses];
  clear_n (new_clauses, new_size_clauses);
  for (uint64_t i = 0; i < size_clauses; i++) {
    for (LratCheckerClause *c = clauses[i], *next; c; c = next) {
      next = c->next;
      const uint64_t h = reduce_hash (c->hash, new_size_clauses);
      c->next = new_clauses[h];
      new_clauses[h] = c;
    }
  }
  delete[] clauses;
  clauses = new_clauses;
  size_clauses = new_size_clauses;
}

// Probably not necessary since we have no watches.
//
void LratChecker::collect_garbage_clauses () {

  stats.collections++;

  LOG ("LRAT CHECKER collecting %" PRIu64 " garbage clauses %.0f%%",
       num_garbage, percent (num_garbage, num_clauses));

  for (LratCheckerClause *c = garbage, *next; c; c = next)
    next = c->next, delete_clause (c);

  assert (!num_garbage);
  garbage = 0;
}

/*------------------------------------------------------------------------*/

LratChecker::LratChecker (Internal *i)
    : internal (i), size_vars (0), strict_lrat (false), concluded (false),
      num_clauses (0), num_finalized (0), num_garbage (0), size_clauses (0),
      clauses (0), garbage (0), last_hash (0), last_id (0), current_id (0) {

  // Initialize random number table for hash function.
  //
  Random random (42);
  for (unsigned n = 0; n < num_nonces; n++) {
    uint64_t nonce = random.next ();
    if (!(nonce & 1))
      nonce++;
    assert (nonce), assert (nonce & 1);
    nonces[n] = nonce;
  }

  strict_lrat = internal ? internal->lrat : 0;

  memset (&stats, 0, sizeof (stats)); // Initialize statistics.
}

void LratChecker::connect_internal (Internal *i) {
  internal = i;
  LOG ("connected to internal");
  strict_lrat = internal->lrat;
}

LratChecker::~LratChecker () {
  LOG ("LRAT CHECKER delete");
  for (size_t i = 0; i < size_clauses; i++)
    for (LratCheckerClause *c = clauses[i], *next; c; c = next)
      next = c->next, delete_clause (c);
  for (LratCheckerClause *c = garbage, *next; c; c = next)
    next = c->next, delete_clause (c);
  delete[] clauses;
}

/*------------------------------------------------------------------------*/

void LratChecker::enlarge_vars (int64_t idx) {

  assert (0 < idx), assert (idx <= INT_MAX);

  int64_t new_size_vars = size_vars ? 2 * size_vars : 2;
  while (idx >= new_size_vars)
    new_size_vars *= 2;
  LOG ("LRAT CHECKER enlarging variables of checker from %" PRId64
       " to %" PRId64 "",
       size_vars, new_size_vars);

  marks.resize (2 * new_size_vars);
  //amar : added this
  checked_lits.resize (2 * new_size_vars);

  assert (idx < new_size_vars);
  size_vars = new_size_vars;
}

inline void LratChecker::import_literal (int lit) {
  assert (lit);
  assert (lit != INT_MIN);
  int idx = abs (lit);
  if (idx >= size_vars)
    enlarge_vars (idx);
  imported_clause.push_back (lit);
}

void LratChecker::import_clause (const vector<int> &c) {
  for (const auto &lit : c)
    import_literal (lit);
}

/*------------------------------------------------------------------------*/

uint64_t LratChecker::reduce_hash (uint64_t hash, uint64_t size) {
  assert (size > 0);
  unsigned shift = 32;
  uint64_t res = hash;
  while ((((uint64_t) 1) << shift) > size) {
    res ^= res >> shift;
    shift >>= 1;
  }
  res &= size - 1;
  assert (res < size);
  return res;
}

uint64_t LratChecker::compute_hash (const uint64_t id) {
  assert (id > 0);
  unsigned j = id % num_nonces;             // dont know if this is a good
  uint64_t tmp = nonces[j] * (uint64_t) id; // hash funktion or if it is
  return last_hash = tmp; // even better than just using id
}

LratCheckerClause **LratChecker::find (const uint64_t id) {
  stats.searches++;
  LratCheckerClause **res, *c;
  const uint64_t hash = compute_hash (id);
  const uint64_t h = reduce_hash (hash, size_clauses);
  for (res = clauses + h; (c = *res); res = &c->next) {
    if (c->hash == hash && c->id == id) {
      break;
    }
    stats.collisions++;
  }
  return res;
}

void LratChecker::insert () {
  stats.insertions++;
  if (num_clauses == size_clauses)
    enlarge_clauses ();
  const uint64_t h = reduce_hash (compute_hash (last_id), size_clauses);
  LratCheckerClause *c = new_clause ();
  c->next = clauses[h];
  clauses[h] = c;
}

/*------------------------------------------------------------------------*/

// TODO "strict" resolution check instead of rup check
bool LratChecker::check_resolution (vector<uint64_t> proof_chain) {
  if (proof_chain.empty ()) { // ignore these case TODO chain.size == 1?
    LOG ("LRAT CHECKER resolution check skipped clause is tautological");
    return true;
  }
  if (strict_lrat) { // ignore this case
    LOG ("LRAT CHECKER resolution check skipped because "
         "opts.externallrat=true");
    return true;
  }
  LOG (imported_clause, "LRAT CHECKER checking clause with resolution");
  for (auto &b : checked_lits)
    b = false; // clearing checking bits
  LratCheckerClause *c = *find (proof_chain.back ());
  assert (c);
  for (int *i = c->literals; i < c->literals + c->size; i++) {
    int lit = *i;
    checked_lit (lit) = true;
    assert (!checked_lit (-lit));
  }
  for (auto p = proof_chain.end () - 2; p >= proof_chain.begin (); p--) {
    auto &id = *p;
    c = *find (id);
    assert (c); // since this is checked in check already
    for (int *i = c->literals; i < c->literals + c->size; i++) {
      int lit = *i;
      if (!checked_lit (-lit))
        checked_lit (lit) = true;
      else
        checked_lit (-lit) = false;
    }
  }
  for (const auto &lit : imported_clause) {
    if (checked_lit (-lit)) {
      LOG ("LRAT CHECKER resolution failed, resolved literal %d in learned "
           "clause",
           lit);
      return false;
    }
    if (!checked_lit (lit)) {
      // learned clause is subsumed by resolvents
      checked_lit (lit) = true;
    }
    checked_lit (-lit) = true;
  }
  for (int64_t lit = 1; lit < size_vars; lit++) {
    bool ok = checked_lit (lit) && checked_lit (-lit);
    ok = ok || (!checked_lit (lit) && !checked_lit (-lit));
    if (!ok) {
      LOG ("LRAT CHECKER resolution failed, learned clause does not match "
           "on "
           "variable %" PRId64,
           lit);
      return false;
    }
  }

  return true;
}

/*------------------------------------------------------------------------*/

bool LratChecker::check (vector<uint64_t> proof_chain) {
  LOG (imported_clause, "LRAT CHECKER checking clause");
  stats.checks++;
  // assert (proof_chain.size ());             // this might be attempting
  // to
  for (auto &b : checked_lits)
    b = false;                              // assert here but fails for
  for (const auto &lit : imported_clause) { // tautological clauses
    checked_lit (-lit) = true;
    if (checked_lit (lit)) {
      LOG (imported_clause, "LRAT CHECKER clause tautological");
      assert (!proof_chain.size ()); // would be unnecessary hence a bug
      return true;
    }
  }
  assert (proof_chain.size ()); // but we can assert it here :)

  vector<LratCheckerClause *> used_clauses;
  bool checking = false;
  for (auto &id : proof_chain) {
    LratCheckerClause *c = *find (id);
    if (!c) {
      LOG ("LRAT CHECKER LRAT failed. Did not find clause with id %" PRIu64,
           id);
      break;
    }
    if (c->tautological) {
      LOG ("LRAT CHECKER LRAT failed. Clause with id %" PRIu64
           " is tautological",
           id);
      break;
    }
    used_clauses.push_back (c);
    if (c->used) {
      LOG ("LRAT CHECKER LRAT failed. Id %" PRIu64
           " was used multiple times",
           id);
      break; // mostly fuzzed and debugged :)
    } else
      c->used = true;
    int unit = 0;
    for (int *i = c->literals; i < c->literals + c->size; i++) {
      int lit = *i;
      if (checked_lit (-lit))
        continue;
      if (unit && unit != lit) {
        unit = INT_MIN; // multiple unfalsified literals
        break;
      }
      unit = lit; // potential unit
    }
    if (unit == INT_MIN) {
      LOG ("LRAT CHECKER check failed, found non unit clause %" PRIu64, id);
      break;
    }
    if (!unit) {
      LOG ("LRAT CHECKER check succeded, clause falsified %" PRIu64, id);
      checking = true;
      break;
    }
    LOG ("LRAT CHECKER found unit clause %" PRIu64 ", assign %d", id, unit);
    checked_lit (unit) = true;
  }
  for (auto &lc : used_clauses) {
    lc->used = false;
  }
  if (!checking) {
    LOG ("LRAT CHECKER failed, no conflict found");
    return false; // check failed because no empty clause was found
  }
  return true;
}

/*------------------------------------------------------------------------*/

void LratChecker::add_original_clause (uint64_t id, bool,
                                       const vector<int> &c, bool restore) {
  START (checking);
  LOG (c, "LRAT CHECKER addition of original clause[%" PRIu64 "]", id);
  if (restore)
    restore_clause (id, c);
  stats.added++;
  stats.original++;
  import_clause (c);
  last_id = id;
  if (!restore && id == 1 + current_id)
    current_id = id;

  if (size_clauses && !restore) {
    LratCheckerClause **p = find (id), *d = *p;
    if (d) {
      fatal_message_start ();
      fputs ("different clause with id ", stderr);
      fprintf (stderr, "%" PRId64, id);
      fputs (" already present\n", stderr);
      fatal_message_end ();
    }
  }
  assert (id);
  insert ();
  imported_clause.clear ();
  STOP (checking);
}

void LratChecker::add_derived_clause (uint64_t id, bool,
                                      const vector<int> &c,
                                      const vector<uint64_t> &proof_chain) {
  START (checking);
  LOG (c, "LRAT CHECKER addition of derived clause[%" PRIu64 "]", id);
  stats.added++;
  stats.derived++;
  import_clause (c);
  last_id = id;
  assert (id == current_id + 1);
  current_id = id;
  if (size_clauses) {
    LratCheckerClause **p = find (id), *d = *p;
    if (d) {
      fatal_message_start ();
      fputs ("different clause with id ", stderr);
      fprintf (stderr, "%" PRId64, id);
      fputs (" already present\n", stderr);
      fatal_message_end ();
    }
  }
  assert (id);
  if (!check (proof_chain) || !check_resolution (proof_chain)) {
    fatal_message_start ();
    fputs ("failed to check derived clause:\n", stderr);
    for (const auto &lit : imported_clause)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  } else
    insert ();
  imported_clause.clear ();
  STOP (checking);
}

void LratChecker::add_assumption_clause (uint64_t id, const vector<int> &c,
                                         const vector<uint64_t> &chain) {
  for (auto &lit : c) {
    if (std::find (assumptions.begin (), assumptions.end (), -lit) !=
        assumptions.end ())
      continue;
    if (std::find (constraint.begin (), constraint.end (), -lit) !=
        constraint.end ())
      continue;
    fatal_message_start ();
    fputs ("clause contains non assumptions or constraint literals\n",
           stderr);
    fatal_message_end ();
  }
  add_derived_clause (id, true, c, chain);
  delete_clause (id, true, c);
  assumption_clauses.push_back (id);
}

void LratChecker::add_assumption (int a) { assumptions.push_back (a); }

void LratChecker::add_constraint (const vector<int> &c) {
  constraint.clear ();
  for (auto &lit : c) {
    assert (lit);
    if (std::find (constraint.begin (), constraint.end (), lit) !=
        constraint.end ())
      continue;
    constraint.push_back (lit);
  }
}

void LratChecker::reset_assumptions () {
  assumption_clauses.clear ();
  assumptions.clear ();
  concluded = false;
  // constraint.clear ();
}

void LratChecker::conclude_unsat (ConclusionType conclusion,
                                  const vector<uint64_t> &ids) {
  if (concluded) {
    fatal_message_start ();
    fputs ("already concluded\n", stderr);
    fatal_message_end ();
  }
  concluded = true;
  if (conclusion == CONFLICT) {
    LratCheckerClause **p = find (ids.back ()), *d = *p;
    if (!d || d->size) {
      fatal_message_start ();
      fputs ("empty clause not in proof\n", stderr);
      fatal_message_end ();
    }
    return;
  } else if (conclusion == ASSUMPTIONS) {
    if (ids.size () != 1 || assumption_clauses.size () != 1) {
      fatal_message_start ();
      fputs ("expected exactly one assumption clause\n", stderr);
      fatal_message_end ();
    }
    if (ids.back () != assumption_clauses.back ()) {
      fatal_message_start ();
      fputs ("conclusion is not an assumption clause\n", stderr);
      fatal_message_end ();
    }
    return;
  } else {
    assert (conclusion == CONSTRAINT);
    if (constraint.size () != ids.size ()) {
      fatal_message_start ();
      fputs ("not complete conclusion given for constraint\n", stderr);
      fputs ("The constraint contains the literals: ", stderr);
      for (auto c : constraint) {
        fprintf (stderr, "%d ", c);
      }

      fputs ("\nThe ids are: ", stderr);
      for (auto c : ids) {
        fprintf (stderr, "%" PRIu64 " ", c);
      }
      fatal_message_end ();
    }
    for (auto &id : ids) {
      if (std::find (assumption_clauses.begin (), assumption_clauses.end (),
                     id) != assumption_clauses.end ())
        continue;
      fatal_message_start ();
      fputs ("assumption clause for constraint missing\n", stderr);
      fatal_message_end ();
    }
  }
}

/*------------------------------------------------------------------------*/

void LratChecker::delete_clause (uint64_t id, bool, const vector<int> &c) {
  START (checking);
  LOG (c, "LRAT CHECKER checking deletion of clause[%" PRIu64 "]", id);
  stats.deleted++;
  import_clause (c);
  last_id = id;
  LratCheckerClause **p = find (id), *d = *p;
  if (d) {
    for (const auto &lit : imported_clause)
      mark (lit) = true;
    const int *dp = d->literals;
    for (unsigned i = 0; i < d->size; i++) {
      int lit = *(dp + i);
      if (!mark (lit)) {        // should never happen since ids
        fatal_message_start (); // are unique.
        fputs ("deleted clause not in proof:\n", stderr);
        for (const auto &lit : imported_clause)
          fprintf (stderr, "%d ", lit);
        fputc ('0', stderr);
        fatal_message_end ();
      }
    }
    for (const auto &lit : imported_clause)
      mark (lit) = false;

    // Remove from hash table, mark as garbage, connect to garbage list.
    num_garbage++;
    assert (num_clauses);
    num_clauses--;
    *p = d->next;
    d->next = garbage;
    garbage = d;
    d->garbage = true;

    // If there are enough garbage clauses collect them.
    // TODO: probably can just delete clause directly without
    // specific garbage collection phase.
    if (num_garbage > 0.5 * max ((size_t) size_clauses, (size_t) size_vars))
      collect_garbage_clauses ();
  } else {
    fatal_message_start ();
    fputs ("deleted clause not in proof:\n", stderr);
    for (const auto &lit : imported_clause)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  }
  imported_clause.clear ();
  STOP (checking);
}

/*------------------------------------------------------------------------*/

void LratChecker::weaken_minus (uint64_t id, const vector<int> &c) {
  LOG (c, "LRAT CHECKER saving clause[%" PRIu64 "] to restore later", id);
  import_clause (c);

  assert (id <= current_id);
  last_id = id;
  LratCheckerClause **p = find (id), *d = *p;
  if (d) {
    for (const auto &lit : imported_clause)
      mark (lit) = true;
    const int *dp = d->literals;
    for (unsigned i = 0; i < d->size; i++) {
      int lit = *(dp + i);
      if (!mark (lit)) {        // should never happen since ids
        fatal_message_start (); // are unique.
        fputs ("deleted clause not in proof:\n", stderr);
        for (const auto &lit : imported_clause)
          fprintf (stderr, "%d ", lit);
        fputc ('0', stderr);
        fatal_message_end ();
      }
    }
    for (const auto &lit : imported_clause)
      mark (lit) = false;
  } else {
    fatal_message_start ();
    fputs ("weakened clause not in proof:\n", stderr);
    for (const auto &lit : imported_clause)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  }
  imported_clause.clear ();

  vector<int> e = c;
  sort (begin (e), end (e));
  clauses_to_reconstruct[id] = e;
}

void LratChecker::restore_clause (uint64_t id, const vector<int> &c) {
  LOG (c, "LRAT CHECKER check of restoration of clause[%" PRIu64 "]", id);
  if (!strict_lrat &&
      clauses_to_reconstruct.find (id) == end (clauses_to_reconstruct)) {
    return;
  }
  if (clauses_to_reconstruct.find (id) == end (clauses_to_reconstruct)) {
    fatal_message_start ();
    fputs ("restoring clauses not deleted previously:\n", stderr);
    for (const auto &lit : c)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  }
  vector<int> e = c;
  sort (begin (e), end (e));
  const vector<int> &d = clauses_to_reconstruct.find (id)->second;
  bool eq = true;
  if (c.size () != d.size ()) {
    eq = false;
  }

  for (std::vector<int>::size_type i = 0; i < e.size () && eq; ++i) {
    eq = (e[i] == d[i]);
  }

  if (!eq) {
    fatal_message_start ();
    fputs ("restoring clause that is different than the one imported:\n",
           stderr);
    for (const auto &lit : c)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fputs ("vs:\n", stderr);
    for (const auto &lit : d)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  }

  clauses_to_reconstruct.erase (id);
}

void LratChecker::finalize_clause (uint64_t id, const vector<int> &c) {
  START (checking);
  LOG (c, "LRAT CHECKER checking finalize of clause[%" PRIu64 "]", id);
  stats.finalized++;
  num_finalized++;
  import_clause (c);
  assert (id <= current_id);
  last_id = id;
  LratCheckerClause **p = find (id), *d = *p;
  if (d) {
    for (const auto &lit : imported_clause)
      mark (lit) = true;
    const int *dp = d->literals;
    for (unsigned i = 0; i < d->size; i++) {
      int lit = *(dp + i);
      if (!mark (lit)) {        // should never happen since ids
        fatal_message_start (); // are unique.
        fputs ("deleted clause not in proof:\n", stderr);
        for (const auto &lit : imported_clause)
          fprintf (stderr, "%d ", lit);
        fputc ('0', stderr);
        fatal_message_end ();
      }
    }
    for (const auto &lit : imported_clause)
      mark (lit) = false;

  } else {
    fatal_message_start ();
    fputs ("deleted clause not in proof:\n", stderr);
    for (const auto &lit : imported_clause)
      fprintf (stderr, "%d ", lit);
    fputc ('0', stderr);
    fatal_message_end ();
  }
  imported_clause.clear ();
  STOP (checking);
}

// check if all clauses have been deleted
void LratChecker::report_status (int, uint64_t) {
  START (checking);
  if (num_finalized == num_clauses) {
    num_finalized = 0;
    LOG ("LRAT CHECKER successful finalize check, all clauses have been "
         "deleted");
  } else {
    fatal_message_start ();
    fputs ("finalize check failed ", stderr);
    fprintf (stderr, "%" PRIu64, num_clauses);
    fputs (" are not finalized", stderr);
    fatal_message_end ();
  }
  STOP (checking);
}

/*------------------------------------------------------------------------*/

void LratChecker::dump () {
  int max_var = 0;
  for (uint64_t i = 0; i < size_clauses; i++)
    for (LratCheckerClause *c = clauses[i]; c; c = c->next)
      for (unsigned i = 0; i < c->size; i++)
        if (abs (c->literals[i]) > max_var)
          max_var = abs (c->literals[i]);
  printf ("p cnf %d %" PRIu64 "\n", max_var, num_clauses);
  for (uint64_t i = 0; i < size_clauses; i++)
    for (LratCheckerClause *c = clauses[i]; c; c = c->next) {
      for (unsigned i = 0; i < c->size; i++)
        printf ("%d ", c->literals[i]);
      printf ("0\n");
    }
}

void LratChecker::begin_proof (uint64_t id) { current_id = id; }

} // namespace CaDiCaL
