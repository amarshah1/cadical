#include "internal.hpp"
#include <map>

namespace CaDiCaL {

void Internal::print_assignment() {
    printf("the current assignment: ");
    for (int i = 1; i <= Internal::max_var; i++) {
        const signed char tmp = val (i);
        if (tmp > 0) {
            printf("%d ", i);
        } else if (tmp < 0) {
            printf("%d ", -1 * i);
        }
    }
    printf("\n");
}

void Internal::least_conditional_part() {
    // bool satisfied = false; // Root level satisfied.

    // use add_new_original_clause (makes it stay forever)
    // be careful about how things get deleted  fi we add as a learned clause (low glue value = less likely to get deleted, below glue of 2, you stay forever)
    // call backtrack with no decisions
    // occurence list maps variables -> clauses : see elim.cpp
    // references 
    printf("HERE:: in the globally blocked addition step \n");
    printf("level %d", level);

    print_assignment ();
    // printf("%d\n", Internal::vals);
    // printf("the current assignment (as s): ");
    // printf("%s\n", Internal::vals);

    // conditional part of the autarky
    vector<int> alpha_c;
    // lit : number of times that literal appears in positive form
    std::map<int, int> times_touched;
    
    // TODO [optimization]: maintain list of clauses touched but not satisfied    
    // TODO [optimization]: keep track of literals pointing to clauses (only look at clauses that have been assigned)
    for (const auto &c: clauses) {
        // printf("Starting oUTER LOOP1!!!");
        
        // TODO [optimization]: keep data vector of irredundant clauses to iterate over
        // skip learned (redundant) clauses
        if (c->redundant) continue;

        // DEBUGGING START
        printf("We are considering the clause: ");
        for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            const int lit = *l;
            printf("%d ", lit);
        }
        printf("\n");
        // DEBUGGING END

        // current assignment satisfies current clause
        bool satisfies_clause = false;
        // assignment mentioned in clause
        vector<int> alpha_touches;

        for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
            const int lit = *l;
            const signed char lit_val = val (lit);
            // printf("here \n");
            if (lit_val > 0) { // positive assignment
                printf("    We satisfy the clause with literal: %d \n", lit);
                satisfies_clause = true;
                // update times_touched with touch
                if (times_touched.find(lit) == times_touched.end()) { // lit not in dict
                    times_touched.insert(std::pair<int, int>(lit, 1));
                } else { // lit in dict
                    times_touched[lit] = times_touched[lit] + 1;
                }
            } else if (lit_val < 0) { // negative assignment
                printf("    got to a false literal : %d \n", lit);
                // add to touched list if not already in alpha_c
                if (!getbit(lit, 1)) {
                    alpha_touches.push_back(lit);
                    printf("    length of alpha_touches is: %d \n", alpha_touches.size());
                }
            }
        }

        // printf("\n    touched wihtout sat is: %d", touched_without_sat);
        if (!satisfies_clause) {
            printf("    we do not satisfy the clause. Alpha_touches size: %d \n", alpha_touches.size());
            // add alpha_touches to alpha_c
            alpha_c.insert(alpha_c.end(), alpha_touches.begin(), alpha_touches.end());
            // set "added to alpha_c" bit
            for (int i=0; i < alpha_touches.size(); i++){
                setbit(alpha_touches[i], 1);
            }
        }
    }

    // TODO [heuristic]: change?
    // pick most touched assigned literal to add as autarky part
    int max_key = 0;
    int max_val = 0;
    for (auto const& [key, val] : times_touched) {
        if (val > max_val && !getbit(key, 1)) {
            max_val = val;
            max_key = key;
        }
    }

    // add literal from autarky part, if it exists
    if (max_key != 0) {
        alpha_c.push_back(max_key);
    }

    // only add a clause of size at least 2
    if (alpha_c.size() > 1) {
        printf("    adding the globally blocked clause: \n");
        printf("\n        length of clause: %d \n", alpha_c.size());
        printf("\n        clause: ");

        for(int i=0; i < alpha_c.size(); i++){
            unsetbit(alpha_c[i], 1);
            printf("%d ", alpha_c[i]);
        }

        printf("\n");

        // printf("\n        printing out alpha: ");

        // for(int i=0; i < alpha.size(); i++){
        // //  for (int& num : alpha) {
        //     printf("%d ", alpha[i]);
        // }

        // printf("\n");

        printf(" one \n");
        clause = alpha_c;
        printf(" two \n");
        backtrack (level - 1);
        printf(" three \n");
        Clause* c = new_learned_redundant_clause(1);
         printf(" four \n");
        // search_assign_driving (-uip, c);
    }

    // LOG("adding the globally blocked clause", alpha_c);


    // Clause* new_clause = new Clause();
    // return alpha_c;
}

bool Internal::globalling () {
  printf("in the globally blocked checking step \n");

  if (!opts.global)
    return false;
  if (!preprocessing && !opts.inprocessing)
    return false;
  if (preprocessing)
    assert (lim.preprocessing);

  // Triggered in regular 'opts.globalint' conflict intervals.
  //
//   if (lim.global > stats.conflicts)
//     return false;

  if (0 == level || level > 3)
    return false; // One decision necessary.

  printf("DOING A GLOBAL CHECK!!! \n");
  global_counter = global_counter + 1;

  if (global_counter % 4 != 0) {
    return false;
  }

//   global_counter = global_counter + 1;


  if (level <= averages.current.jump)
//     return false; // Main heuristic.

  return true;

//   if (!stats.current.irredundant)
//     return false;
//   double remain = active ();
//   if (!remain)
//     return false;
//   double ratio = stats.current.irredundant / remain;
//   return ratio <= opts.globalmaxrat;
}

void Internal::global () {

  if (unsat)
    return;
  if (!stats.current.irredundant)
    return;

//   START_SIMPLIFIER (global, global);
//   stats.globalings++;

  // Propagation limit to avoid too much work in 'global'.  We mark
  // tried candidate clauses after giving up, such that next time we run
  // 'global' we can try them.
  //
//   long limit = stats.propagations.search;
//   limit *= opts.globalreleff;
//   limit /= 1000;
//   if (limit < opts.globalmineff)
//     limit = opts.globalmineff;
//   if (limit > opts.globalmaxeff)
//     limit = opts.globalmaxeff;
//   assert (stats.current.irredundant);
//   limit *= 2.0 * active () / (double) stats.current.irredundant;
//   limit = max (limit, 2l * active ());

//   PHASE ("global", stats.globalings,
//          "started after %" PRIu64 " conflicts limited by %ld propagations",
//          stats.conflicts, limit);

//   long blocked = global_round (limit);

//   STOP_SIMPLIFIER (global, global);
//   report ('g', !blocked);

//   if (!update_limits)
//     return;

//   long delta = opts.globalint * (stats.globalings + 1);
//   lim.global = stats.conflicts + delta;

//   PHASE ("global", stats.globalings,
//          "next limit at %" PRIu64 " after %ld conflicts", lim.global,
//          delta);
}

    

} // namespace CaDiCaL
