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




    vector<int> alpha_c;
    std::map<int, int> times_touched;

    // vector<int> alpha;

    
    // todo : maintain a list of clauses that are touched but not satisfied (optimization for later)
    // only need to go over irredundant clauses
    // if c->irredundmnnt check [might be worth to keep a seperate vector of original clauses]

    // todo: [optimization] keep track of literals pointing to clauses (only look at clauses that have been assigned)
    for (const auto &c: clauses) {

        // printf("Starting oUTER LOOP1!!!");

        // skip the learned clauses
        // might be worth have a separate data structure so we only enumerate over these
        // if (c->redundant) {
        //     // printf("breaking through a redundant clause");
        //     continue;
        // }

        printf("We are considering the clause: ");

        for(const_literal_iterator l = c->begin (); l != c->end (); l++){
        //  for (int& num : alpha_c) {
            const int lit = *l;
            printf("%d ", lit);
        }
        printf("\n");

        bool satisfies_clause = false;
        vector<int> alpha_touches;

        for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
            const int lit = *l;
            const signed char tmp = val (lit);
            // printf("here \n");
            if (tmp > 0) {
                printf("    We satisfy the clause with literal: %d \n", lit);
                satisfies_clause = true;
                // increase the number of times this is touched
                if (times_touched.find(lit) == times_touched.end()) {
                    // not found
                    times_touched.insert(std::pair<int, int>(lit, 1));
                } else {
                    times_touched[lit] = times_touched[lit] + 1;
                }
            } else if (tmp < 0) {
                printf("    got to a false literal : %d \n", lit);
                // touched_without_sat = true;
                // but also only want to add it if it has not already been added to alpha_c — to getbit check
                if (!getbit(lit, 1)) {
                    alpha_touches.push_back(lit);
                    printf("    length of alpha_touches is: %d \n", alpha_touches.size());
                    // setbit(lit, 1); // todo : this is wrong
                }
            }
        }

        // printf("\n    touched wihtout sat is: %d", touched_without_sat);
        if (!satisfies_clause) {
            printf("    we do not satisfy the clause. Alpha_touches size: %d \n", alpha_touches.size());
            for (int i=0; i < alpha_touches.size(); i++){
                setbit(alpha_touches[i], 1);
            }
            alpha_c.insert(alpha_c.end(), alpha_touches.begin(), alpha_touches.end());
        }
        
    }
    // set the clause to alpha_c
    // assert (alpha)

    int max_key = 0;
    int max_val = 0;
    for (auto const& [key, val] : times_touched)
        {
            if (val > max_val && !getbit(key, 1)) {
                max_val = val;
                max_key = key;
            }
        }

    if (max_key != 0) {
        alpha_c.push_back(max_key);
    }

    

    if (alpha_c.size() > 0) {
        printf("    adding the globally blocked clause: \n");

    //     for(const int& i : vi) 
    //   cout << "i = " << i << endl


        printf("\n        length of clause: %d \n", alpha_c.size());
        printf("\n        clause: ");

        for(int i=0; i < alpha_c.size(); i++){
        //  for (int& num : alpha_c) {
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
