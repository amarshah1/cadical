#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
using namespace std;

namespace CaDiCaL {

bool print_out = false;

void Internal::print_assignment() {
    LOG("the current assignment: ");
    for (int i = 1; i <= Internal::max_var; i++) {
        const signed char tmp = val (i);
        if (tmp > 0) {
            LOG("%d ", i);
        } else if (tmp < 0) {
            LOG("%d ", -1 * i);
        }
    }
    LOG("\n");
}

void Internal::print_clause (CaDiCaL::Clause *const &c) {
    // for(const_literal_iterator l = c->begin (); l != c->end (); l++){
    //         const int lit = *l;
    //         printf("%d ", lit);
    //     }
}

void Internal::print_all_clauses() {
    printf ("Printing all clauses: \n");
    for (const auto &c: clauses) {
        printf( "    ");
        print_clause (c);
        printf("\n");
    }
}

int Internal::length_of_current_assignment() {
    int length = 0;
    for (int i = 1; i <= Internal::max_var; i++) {
        const signed char tmp = val (i);
        if (tmp != 0) {
            length += 1;
        }
    }
    return length;
}

void Internal::sort_vec_by_decision_level(vector<int>* v) {
    LOG("the clause before: ");
    for(int i=0; i < v->size(); i++){
            LOG("%d ", (*v)[i]);
        }
    std::sort(v->begin (), v->end (), 
                [this](int x, int y) {
                    return (var (x).level > var (y).level);
                });
    LOG("\n the clause after: ");
    for(int i=0; i < v->size(); i++){
            LOG("%d [%d] ", (*v)[i], var ((*v)[i]).level);
        }
    LOG("\n");
}

void Internal::least_conditional_part() {
    // bool satisfied = false; // Root level satisfied.

    // use add_new_original_clause (makes it stay forever)
    // be careful about how things get deleted  fi we add as a learned clause (low glue value = less likely to get deleted, below glue of 2, you stay forever)
    // call backtrack with no decisions
    // occurence list maps variables -> clauses : see elim.cpp
    // references 
    LOG("HERE:: in the globally blocked addition step \n");
    LOG("level %d", level);

    print_assignment ();
    // LOG("%d\n", Internal::vals);
    // LOG("the current assignment (as s): ");
    // LOG("%s\n", Internal::vals);

    // conditional part of the autarky
    vector<int> alpha_c;
    // lit : number of times that literal appears in positive form
    std::map<int, int> times_touched;
    
    // TODO [optimization]: maintain list of clauses touched but not satisfied    
    // TODO [optimization]: keep track of literals pointing to clauses (only look at clauses that have been assigned)
    for (const auto &c: clauses) {        
        // skip learned (redundant) clauses (but only when we are in proof checking mode)
        // todo: need to
        if (c->redundant && !proof) continue;

        // DEBUGGING START
        LOG("We are considering the clause: ");
        for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            const int lit = *l;
            LOG("%d ", lit);
        }
        LOG("\n");
        // DEBUGGING END

        // current assignment satisfies current clause
        bool satisfies_clause = false;
        // assignment mentioned in clause
        vector<int> alpha_touches;

        for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
            const int lit = *l;
            const signed char lit_val = val (lit);
            // LOG("here \n");
            if (lit_val > 0) { // positive assignment
                LOG("    We satisfy the clause with literal: %d \n", lit);
                satisfies_clause = true;
                // update times_touched with touch
                if (times_touched.find(lit) == times_touched.end()) { // lit not in dict
                    times_touched.insert(std::pair<int, int>(lit, 1));
                } else { // lit in dict
                    times_touched[lit] = times_touched[lit] + 1;
                }
            } else if (lit_val < 0) { // negative assignment
                LOG("    got to a false literal : %d \n", lit);
                // add to touched list if not already in alpha_c
                if (!getbit(lit, 0)) {
                    alpha_touches.push_back(lit);
                    LOG("    length of alpha_touches is: %d \n", alpha_touches.size());
                }
            }
        }

        // LOG("\n    touched wihtout sat is: %d", touched_without_sat);
        if (!satisfies_clause) {
            LOG("    we do not satisfy the clause. Alpha_touches size: %d \n", alpha_touches.size());
            // add alpha_touches to alpha_c
            alpha_c.insert(alpha_c.end(), alpha_touches.begin(), alpha_touches.end());
            // set "added to alpha_c" bit
            for (int i=0; i < alpha_touches.size(); i++){
                setbit(alpha_touches[i], 0);
            }
        }
    }

    // keep track of this for proof
    vector<int> alpha_a_sans_lit;

    vector<vector<int>> clauses_to_add;

    // Marijn suggested the heuristic that when alpha_a is entirely decision literals,
    // we need to not add a clause
    // bool alpha_a_entirely_decision_literals = true;

    // TODO [heuristic]: change?
    // pick most touched assigned literal to add as autarky part
    int max_key = 0;
    int max_val = 0;
    printf("assigment: ");
    for (auto const& [key, val] : times_touched) {
        // if (is_decision (key))
        printf("%d [%d] ", key, var (key).level);
        if (is_decision (key))
            printf("[d]  ");


        if (!getbit(key, 0)) {

            if (!is_decision (key)) {
                vector<int> new_clause = alpha_c;
                new_clause.push_back(key);
                clauses_to_add.push_back(new_clause);
            } else {
                printf("[AUTARKY DECISION ALERT: %d]", key);
            }

            if (val > max_val && !is_decision (key)) {
                if (max_key != 0) {
                    LOG("adding to alpha_a_sans_lit (version 1): %d\n", max_key);
                    alpha_a_sans_lit.push_back(max_key);
                }
                max_val = val;
                max_key = key;
            } else 
                alpha_a_sans_lit.push_back(key);
        }
    }
    printf("\n");

    if (print_out) {
        int alpha_c_decision = 0;

        for (int i=0; i < alpha_c.size(); i++) {
            if (is_decision (alpha_c[i]))
                alpha_c_decision++;
        }

        int alpha_a_decision = 0;

        if (max_key && is_decision (max_key)) {
            alpha_a_decision += 1;
        }

        for (int i=0; i < alpha_a_sans_lit.size(); i++) {
            if (is_decision (alpha_a_sans_lit[i]))
                alpha_a_decision++;
        }


        std::ofstream file("tmp2.txt", std::ios::app);  // Open file in append mode

        // Check if the file opened successfully
        if (!file) {
            LOG("Error opening file for writing!");
            return;
        }
        else {

            // Write x and y in "x, y" format
            file << alpha_c.size() << ", " << alpha_a_sans_lit.size() << "," << max_key << ", " << (length_of_current_assignment ()) << ", " << alpha_c_decision << ", " << alpha_a_decision << ", " << level << std::endl;

            file.close();  // Close the file
        }
        }

    

    // have to unset all of the bits
    for(int i=0; i < alpha_c.size(); i++){
        // don't want to unset the max_key thing
        // if (alpha_c[i] != max_key)
        unsetbit(alpha_c[i], 0);
        // LOG("%d ", alpha_c[i]);
    }

    // it is unsound add globally blocked clause when alpha_c = alpha
    if (max_key == 0) 
        return;

    // if alpha_a is entirely decision literals, then we should not return a clause
    // if (alpha_a_entirely_decision_literals) {
    //     printf("This is entirely decision literals");
    //     return;
    // }

    // add literal from autarky part, if it exists
    LOG("ADDING THE MAX_KEY %d \n", max_key);
    vector<int> new_clause = alpha_c;
    new_clause.push_back(max_key);


    for (int i=0; i < clauses_to_add.size(); i++){
        // only add a clause of size at least 2

        vector<int> new_clause = clauses_to_add[i];

        if (new_clause.size() > 1) {
            // LOG("\n        printing out alpha: ");

            // for(int i=0; i < alpha.size(); i++){
            // //  for (int& num : alpha) {
            //     LOG("%d ", alpha[i]);
            // }

            // LOG("\n");

            printf("    adding the globally blocked clause: ");
            for(int i=0; i < new_clause.size(); i++){
                printf("%d[%d] ", new_clause[i], var (new_clause[i]).level);
            }
            printf("\n        length of clause: %d \n", new_clause.size());

            printf("\n");

            LOG(" one \n");
            sort_vec_by_decision_level(&new_clause);
            clause = new_clause;
            LOG(" two \n");
            backtrack ();
            LOG(" three \n");
            // todo: comment this back in to add clauses
            Clause* c = new_learned_irredundant_global_clause (max_key, alpha_c, alpha_a_sans_lit, 1);
            clause.clear ();

            
            // search_assign_driving (-uip, c);
        }
    }

    // LOG("adding the globally blocked clause", alpha_c);


    // Clause* new_clause = new Clause();
    // return alpha_c;
}

bool Internal::globalling () {
  LOG("in the globally blocked checking step \n");

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

  if (0 == level || level > 5)
    return false; // One decision necessary.

  LOG("DOING A GLOBAL CHECK!!! \n");
  global_counter = global_counter + 1;

  // runtime for  
  if (global_counter % 8 != 0) {
    return false;
  }

//   global_counter = global_counter + 1;


//   if (level <= averages.current.jump)
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


    

} // namespace CaDiCaL
