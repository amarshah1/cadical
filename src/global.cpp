#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
using namespace std;

namespace CaDiCaL {

bool print_out = true;

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
    printf("the clause before: ");
    for(int i=0; i < v->size(); i++){
            printf("%d ", (*v)[i]);
        }
    std::sort(v->begin (), v->end (), 
                [this](int x, int y) {
                    return (var (x).level > var (y).level);
                });
    printf("\n the clause after: ");
    for(int i=0; i < v->size(); i++){
            printf("%d [%d] ", (*v)[i], var ((*v)[i]).level);
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
                if (!getbit(lit, 0)) {
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
                setbit(alpha_touches[i], 0);
            }
        }
    }

    // TODO [heuristic]: change?
    // pick most touched assigned literal to add as autarky part
    int max_key = 0;
    int max_val = 0;
    for (auto const& [key, val] : times_touched) {
        if (val > max_val && !getbit(key, 0)) {
            max_val = val;
            max_key = key;
        }
    }

    // add literal from autarky part, if it exists
    if (max_key != 0) {
        printf("ADDING THE MAX_KEY %d \n", max_key);
        alpha_c.push_back(max_key);
    }

    // have to unset all of the bits
    for(int i=0; i < alpha_c.size(); i++){
        // don't want to unset the max_key thing
        if (alpha_c[i] != max_key)
            unsetbit(alpha_c[i], 0);
        // printf("%d ", alpha_c[i]);
    }

    // only add a clause of size at least 2
    if (alpha_c.size() > 1) {
        printf("    adding the globally blocked clause: ");
        for(int i=0; i < alpha_c.size(); i++){
            printf("%d ", alpha_c[i]);
        }
        printf("\n        length of clause: %d \n", alpha_c.size());
        printf("        clause: ");


        printf("\n");

        // printf("\n        printing out alpha: ");

        // for(int i=0; i < alpha.size(); i++){
        // //  for (int& num : alpha) {
        //     printf("%d ", alpha[i]);
        // }

        // printf("\n");

        // will print length of alpha_c and total assignment to some text file
        if (print_out) {
            std::ofstream file("tmp.txt", std::ios::app);  // Open file in append mode

            // Check if the file opened successfully
            if (!file) {
                printf("Error opening file for writing!");
                return;
            }
            else {

                // Write x and y in "x, y" format
                file << alpha_c.size() << ", " << (length_of_current_assignment ()) << std::endl;

                file.close();  // Close the file
            }
        }


        printf(" one \n");
        sort_vec_by_decision_level(&alpha_c);
        clause = alpha_c;
        printf(" two \n");
        // backtrack (level - 1);
        printf(" three \n");
        // todo: comment this back in to add clauses
        Clause* c = new_learned_redundant_clause (1);
        clause.clear ();
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
