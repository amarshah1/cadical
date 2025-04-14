#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <set>
#include <utility>  // For std::pair
using namespace std;

namespace CaDiCaL {

bool print_out = false;

void Internal::print_assignment() {
    printf("the current assignment: ");
    vector<int> decisions = vector<int> ();
    int assignment_length = 0;
    for (int i = 1; i <= Internal::max_var; i++) {
        const signed char tmp = val (i);
        if (tmp > 0) {
            assignment_length +=1 ;
            if (is_decision (i)) {
                decisions.push_back(i);
                printf("%d [d] ", i);
            } else {
                printf("%d ", i);
            }

        } else if (tmp < 0) {
            assignment_length +=1 ;
            if (is_decision (-i)) {
                decisions.push_back(-i);
                printf("%d [d] ", -i);
            } else {
                printf("%d ", -i);
            }
        }
    }
    printf("\n");
    printf("decisions: ");
    for (int i = 0; i < decisions.size (); i++) {
        printf("%d ", decisions[i]);
    }

    vector<int> fixed_literals = vector<int>();
    get_all_fixed_literals (fixed_literals);
    printf("\n");
    printf("fixed_literals: ");
    for (int i = 0; i < fixed_literals.size (); i++) {
        printf("%d ", fixed_literals[i]);
    }
    printf("assignment length: %d; fixed_literals length : %d \n ", assignment_length, fixed_literals.size ());
}

void Internal::print_clause (CaDiCaL::Clause *const &c) {
    // printf("Entering print_clause\n");
    // if (!c)
    //     printf("c is a null pointer!\n");
    assert (c); // don't want a null pointer
    for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            const int lit = *l;
            printf("%d ", lit);
        }
    printf("\n");
}

// // Custom comparison logic as a non-static member function
bool Internal::compare_alpha_a(int a, int b) {
    printf("comparing %d and %d\n", a, b);
    backtrack ();
    search_assume_decision(a);

    if (!propagate()) {
        // we should never get here since we are propagating only from alpha_a
        assert (false);
        // std::cout << "We reach a conflict from a single propagation on " << a << " and will analyze\n";
        // analyze();

        // if (!propagate()) {
        //     analyze();
        //     std::cout << "We should have reached a contradiction!\n";
        // }
    } else if (val(b) > 0) {
        printf("We get that %d < %d\n", a, b);
        return true;  // a should come before b
    }

    return false;  // Otherwise, maintain original order
}

//custom sort function as I cannot use std::sort to sort alpha_a since compare_alpha_a is non-statics
// basically does a naive swapping thing
// need to make this more efficient and do a topological sort
void Internal::custom_sort_alpha_a(std::vector<int>& alpha_a) {
    for (size_t i = 0; i < alpha_a.size(); ++i) {
        for  (size_t j = i + 1; j < alpha_a.size(); ++j) {
            if (compare_alpha_a(alpha_a[j] , alpha_a[i])) {
                printf("comparing %d and %d\n", alpha_a[i], alpha_a[j]);
                int temp = alpha_a[j];
                alpha_a[j] = alpha_a[i];
                alpha_a[i] = temp;
            }
        }
    }
}

void printSet(const unordered_set<int> uset) {
    printf("  { ");
        for (const auto& elem : uset) {
            printf("%d ", elem);
        }
        printf("}\n");
}


void printVectorOfSets(const vector<unordered_set<int>>& vec) {
    printf("[\n");
    for (const auto& uset : vec) {
        printSet(uset);
    }
    printf("]\n");
}


// todo: need to double check that this works
// Function to find the smallest number of subsets that maximize union
pair<vector<int>, vector<int>> greedySetCover(vector<unordered_set<int>>& subsets, vector<int> total_elements) {
    printf("We are in greedy_set_cover!\n");
    

    unordered_set<int> covered;
    unordered_set<int> uncovered(total_elements.begin(), total_elements.end());;
    int num_elements = 0;
    // set<int> all_elements;
    vector<int> chosen_subsets;
    
    // Collect all elements from the subsets
    // for (const auto& subset : subsets) {
    //     uncovered.insert(subset.begin(), subset.end());
    //     num_elements += 1;
    // }


    
    // Greedy selection process
    while (covered.size() < total_elements.size ()) {
        printf("New iteration!");
        int best_idx = -1;
        size_t max_new_elements = 0;
        
        // Find the subset that adds the most new elements
        for (size_t i = 0; i < subsets.size(); i++) {
            size_t new_elements = 0;
            for (int elem : subsets[i]) {
                if (covered.find(elem) == covered.end()) {
                    new_elements++;
                }
            }
            
            if (new_elements > max_new_elements) {
                max_new_elements = new_elements;
                best_idx = i;
            }
        }

        // No more useful subsets found
        if (best_idx == -1) break;

        printf("We have found the best subset with id %d: \n", best_idx);
        printSet(subsets[best_idx]);
        
        
        
        // Add the chosen subset to the solution
        chosen_subsets.push_back(best_idx);
        covered.insert(subsets[best_idx].begin(), subsets[best_idx].end());
        for (const auto& elem : subsets[best_idx]) {
            uncovered.erase(elem);
            printf("erasing %d from uncovered!\n", elem);
            printSet(uncovered);
        }
    }

    vector<int> uncovered_vector(uncovered.begin (), uncovered.end ());


    
    return std::make_pair(chosen_subsets, uncovered_vector);
}



// todo : need to write this
pair<vector<int>, vector<int>> Internal::greedy_sort_alpha_a(std::vector<int> alpha_a, std::vector<int> neg_alpha_c) {

    printf("doing a greedy_sort_alpha_a with: \n");
    printf("alpha_a: ");
    print_vector(alpha_a);
    printf("neg_alpha_c");
    print_vector(neg_alpha_c);

    vector<int> alpha_a_useful;
    vector<unordered_set<int>> alpha_a_propagated;

    for (int i=0; i < alpha_a.size(); i++){
            printf("Trying to shrink using %d\n", alpha_a[i]);
            Flags &f = flags (alpha_a[i]);

            if (f.status == Flags::FIXED) {
                continue;
            }   

            backtrack (0);
            search_assume_decision(-alpha_a[i]); 

            if (!propagate ()) {
                printf("We got a conflict when propagating from %d\n", -alpha_a[i]);
                analyze ();

                if (unsat)
                    break;
                    
                if (!propagate ()) {
                    printf ("got to a conflict \n");
                    analyze ();
                    break;
                }
                continue;
            } 
            unordered_set<int> propagated;
            for (int j=0; j < neg_alpha_c.size(); j ++) {
                int v = val (neg_alpha_c[j]);
                if (v < 0) {
                    printf("The literal %d in ~alpha_a implies literal %d in alpha_c by unit propagation \n", -alpha_a[i], -neg_alpha_c[j]);
                    assert (j < neg_alpha_c.size());
                    propagated.insert(neg_alpha_c[j]);
                }
            }
            if (propagated.size () > 0) {
                alpha_a_useful.push_back(alpha_a[i]);
                alpha_a_propagated.push_back(propagated);
            }
        }
    // was remembering literals without this backtrack
    backtrack (0);

    printf("We are calling greedySsetCover!\n ");
    printf("Subsets:");
    printVectorOfSets(alpha_a_propagated);
    printf("neg_alpha_c:");
    print_vector (neg_alpha_c);

    auto [chosen_indices, neg_alpha_c_without_c0] = greedySetCover(alpha_a_propagated, neg_alpha_c);

    vector<int> alpha_a_useful_final;

    // Add elements from alpha_a_useful based on chosen_indices
    for (int index : chosen_indices) {
        alpha_a_useful_final.push_back(alpha_a_useful[index]);
    }

    printf("We are returning alpha_a_useful_final: ");
    print_vector (alpha_a_useful_final);

    printf("We are returning neg_alpha_c_without_c0: ");
    print_vector (neg_alpha_c_without_c0);


    return std::make_pair(alpha_a_useful_final, neg_alpha_c_without_c0);
}

void Internal::print_vector (vector<int> c) {
    for(int i = 0; i < c.size(); i++){
            printf("%d ", c[i]);
        }
    // printf("\n");
}

void Internal::print_all_clauses() {
    printf ("Printing all clauses: \n");
    for (const auto &c: clauses) {
        print_clause (c);
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
    // for(int i=0; i < v->size(); i++){
    //         LOG("%d ", (*v)[i]);
    //     }
    std::sort(v->begin (), v->end (), 
                [this](int x, int y) {
                    return (var (x).level > var (y).level);
                });
    LOG("\n the clause after: ");
    // for(int i=0; i < v->size(); i++){
    //         LOG("%d [%d] ", (*v)[i], var ((*v)[i]).level);
    //     }
    LOG("\n");
}

int min(int i, int j) {
    if (i < j)
        return i;
    else
        return j;
}

bool Internal::check_if_clause_trivial(vector<int> c) {
    START (trivial);
    printf("checking if clause is trivial: ");
    print_vector (c);
    backtrack ();
    bool is_trivial = false;
    for (auto lit : c) {
        if (val (lit) > 0) {
            printf("Literal %d is trivially true\n", lit);
            is_trivial = true;
            break;
        } else if (val (lit) < 0) {
            printf("Literal %d is trivially false\n", lit);
            // is_trivial = true;
            continue;
        }
        search_assume_decision (-1 * lit);
        
        if (!propagate ()) {
            printf("found a conflict on %d!\n", lit);
            analyze (false); // todo: made these false right now to avoid bug, but will have to check later
            while (!unsat && !propagate ()) {
                printf("found another conflict!\n");
                analyze (false);
            }
            printf("finished with conflicts!\n");
            is_trivial = true;
            break;
        }
    }
    backtrack ();
    STOP (trivial);
    printf("Finished check trivial!\n");
    return is_trivial;
}

bool Internal::least_conditional_part(std::ofstream& outFile, std::ofstream& outFile_pr) {

    START (global);

    // printf("we have propagated: %d \n", propagated);
    // printf("we have trail.size: %d \n", trail.size ());
    // print_assignment();
    // bool satisfied = false; // Root level satisfied.

    // use add_new_original_clause (makes it stay forever)
    // be careful about how things get deleted  fi we add as a learned clause (low glue value = less likely to get deleted, below glue of 2, you stay forever)
    // call backtrack with no decisions
    // occurence list maps variables -> clauses : see elim.cpp
    // references 
    LOG("HERE:: in the globally blocked addition step \n");
    LOG("level %d", level);

    LOG("here are all the clauses:");

    for (const auto &c: clauses) {    
        LOG(c, "clause: ");
    }    

    LOG(assumptions, "here are the assumptions:");


    // conditional part of the autarky
    vector<int> neg_alpha_c;
    // lit : number of times that literal appears in positive form
    std::map<int, int> times_touched;
    
    // TODO [optimization]: maintain list of clauses touched but not satisfied    
    // TODO [optimization]: keep track of literals pointing to clauses (only look at clauses that have been assigned)
    for (const auto &c: clauses) {        
        // skip learned (redundant) clauses (but only when we are in proof checking mode)
        // todo: need to
        if (c->redundant && !proof) continue;

        // DEBUGGING START
        // LOG("AWe are considering the clause: ");
        // for(auto lit : *c){
        //     LOG("%d ", lit);
        // }
        // LOG("\n");
        // DEBUGGING END

        // current assignment satisfies current clause
        bool satisfies_clause = false;
        // assignment mentioned in clause
        vector<int> alpha_touches;


        // basically will skip a clause that should be deleted, 
        // below commented out old way since it is less effiecient
        // need to double check that what I have rn works
        if (c->garbage) {
            // printf("skipping clause: ");
            // print_clause (c);
            continue;
        }
        bool skip_clause = false;
        for (const_literal_iterator l = c->begin (); l != c->end (); l++) {
            const int lit = *l;
            const signed char lit_val = val (lit);
            Flags &f = flags (lit);
            if (lit_val > 0 && f.status == Flags::FIXED) {
                skip_clause = true;
                break;
            }
        } 
        if (skip_clause) {
            // printf("skipping clause2 of size %d: ", c->size);
            // print_clause (c);
            // printf("\n");
            continue;
        }
        // printf("got past the skip_clause2!\n");
        // print_clause (c);

        // if (c->size == 3) {
        //     const_literal_iterator l = c->begin ();
        //     const int lit = *l;
        //     const_literal_iterator l2 = l + 1;
        //     const int lit2 = *l2;
        //     const_literal_iterator l3 = l + 2;
        //     const int lit3 = *l3;
        //     if ((lit == -2476044) && (lit2 == -24211) && (lit3 == -2474496)) {
        //         if (val (24211) < 0 && val(-2476032) > 0 && val (2469759) < 0 && val (-2476031) > 0 && val (-2476030) > 0 && val (-2476029) > 0 && val (-24104) > 0) {
        //             raise(SIGTRAP);  // Trigger a breakpoint in gdb
        //         }
        //     }
        // }

        // fflush(stdout);

        for (auto lit : *c) {
            const signed char lit_val = val (lit);
            if (lit_val > 0) { // positive assignment
                // printf("    We satisfy the clause with literal: %d \n", lit);
                satisfies_clause = true;
                // update times_touched with touch
                if (times_touched.find(lit) == times_touched.end()) { // lit not in dict
                    times_touched.insert(std::pair<int, int>(lit, 1));
                } else { // lit in dict
                    times_touched[lit] = times_touched[lit] + 1;
                }
            } else if (lit_val < 0) { // negative assignment

                // skip the fixed negative literals
                Flags &f = flags (lit);
                if (f.status == Flags::FIXED) {
                    continue;
                }
                // printf("    got to a false literal : %d for clause: ", lit);
                // fflush(stdout);
                // print_clause (c);
                // add to touched list if not already in neg_alpha_c
                if (!getbit(lit, 0)) {
                    alpha_touches.push_back(lit);
                    // printf("    length of alpha_touches is: %d \n", alpha_touches.size());
                    // fflush(stdout);
                }
            }
        }

        // LOG("\n    touched wihtout sat is: %d", touched_without_sat);
        if (!satisfies_clause) {
            // printf("    we do not satisfy the clause. Alpha_touches size: %d \n", alpha_touches.size());
            // fflush(stdout);
            // add alpha_touches to neg_alpha_c
            neg_alpha_c.insert(neg_alpha_c.end(), alpha_touches.begin(), alpha_touches.end());
            // set "added to neg_alpha_c" bit
            for (int i=0; i < alpha_touches.size(); i++){
                // printf("adding to neg_alpha_c: %d \n", alpha_touches[i]);
                setbit(alpha_touches[i], 0);
            }
        }
    }

    // keep track of this for proof
    vector<int> alpha_a;

    // negate the decision variables
    // todo: this is a hack to get a quick unsat, but we cannot keep it
    // vector<int> conflict_clause;

    vector<vector<int>> clauses_to_add;

    // Marijn suggested the heuristic that when alpha_a is entirely decision literals,
    // we need to not add a clause
    // bool alpha_a_entirely_decision_literals = true;

    // TODO [heuristic]: change?
    // pick most touched assigned literal to add as autarky part
    // int max_key = 0;
    // int max_val = 0;
    for (auto const& [key, val] : times_touched) {
        // if (is_decision (key)) {
        //     conflict_clause.push_back(-1 * key);
        // }


        if (!getbit(key, 0)) {

            if (!is_decision (key)) {
                vector<int> new_clause = neg_alpha_c;
                new_clause.push_back(key);
                clauses_to_add.push_back(new_clause);
            } else {
                // // printf("[AUTARKY DECISION ALERT: %d]", key);
            }

            // if (val > max_val && !is_decision (key)) {
            //     max_val = val;
            //     max_key = key;
            // } 
            int key_val = Internal::val (key);
            // printf("adding %d with value %d to alpha_a with val %d \n", key, val, key_val);
            alpha_a.push_back(key);
        }
    }

    // have to unset all of the bits
    for(int i=0; i < neg_alpha_c.size(); i++){
        unsetbit(neg_alpha_c[i], 0);
    }


    // it is unsound add globally blocked clause when neg_alpha_c = alpha
    // if (max_key == 0) {
    //     STOP (global);
    //     // printf("We are exiting: %d\n", clauses_to_add.size());
    //     return false;
    // }


    vector <int>neg_alpha_c_minus_c0(neg_alpha_c);
    // todo: try to remember why we start alpha_a_useful as alpha_a and shrink vs the opposite
    vector <int>alpha_a_useful;

    int useful_alpha_i = 0;

    // // printf(neg_alpha_c_minus_c0, "We start with neg_alpha_c_minus_c0:");
    // // printf(alpha_a, "We start with alpha_a:");
    // for (int i =0; i < alpha_a.size (); i++) {
    //     if (is_decision (alpha_a[i])) {
    //         LOG("is decision: %d", i);
    //     }
    // }

    Var minus_nineteen_var = var (-19);
    Clause *minus_nineteen_reason = minus_nineteen_var.reason;

    if (minus_nineteen_reason) {
        printf("2. The reason for -19 is: \n");
        print_clause(minus_nineteen_reason);
    }

    printf("\n");
    printf("We are at level: %d \n", level);

    print_assignment ();

    printf("We have alpha_a before sort:");
    print_vector(alpha_a);



    // putting alpha_a in a random order for rn; todo: get rid of this
    // std::random_device rd;  // Non-deterministic random number source
    // std::default_random_engine rng(rd());  // Seeding the engine

    // std::shuffle(alpha_a.begin(), alpha_a.end(), rng);

    // sorting alpha_a in ascending order ~> dumb trick to get pigeonhole to work (todo: add this back)

    if (opts.globalalphaagreedy) {
        custom_sort_alpha_a(alpha_a);
        printf("Pre-entering a gread_sort_alpha_a");
        std::tie(alpha_a_useful, neg_alpha_c_minus_c0) = greedy_sort_alpha_a(alpha_a, neg_alpha_c); // issue with variable shadowing

        printf("We have alpha_a_useful:");
        print_vector(alpha_a_useful);
        printf("\n");

        printf("We have neg_alpha_c_minus_c0:");
        print_vector(neg_alpha_c_minus_c0);
        printf("\n");
    } else {

        printf("We are entering the else case!\n");

        if (opts.globalalphaasort) {
            custom_sort_alpha_a(alpha_a);
        }



        


        
        //  [](int a, int b, int backtrack) {
        //     // if (a % 10 == 0) {
        //     //     return true;
        //     // } else if (b % 10 == 0) {
        //     //     return false;
        //     // }
        //     // return std::abs(a) < std::abs(b);
        //     backtrack ();
        // });

        printf("We have alpha_a after sort:");
        print_vector(alpha_a);
        printf("\n");

        printf("We have neg_alpha_c before:");
        print_vector(neg_alpha_c);
        printf("\n");
        // fflush(stdout);




        printf("shrinking: \n");
        // try to shrink clauses using binary clause propagation, instead of a more general propagation
        if (opts.globalbcp) {
            for (int i=0; i < alpha_a.size(); i++){

                // printf("We are are on %d", alpha_a[i]);

                // want to skip over fixed literals using something like this
                // todo : I think this shouldn't be necessary because we checked it earlier when creating alpha_a
                Flags &f = flags (alpha_a[i]);

                if (f.status == Flags::FIXED) {
                    // printf("We are skipping %d \n", alpha_a[i]);
                    continue;
                }   

                Watches &ws = watches (alpha_a[i]);
                const const_watch_iterator end = ws.end ();
                watch_iterator j = ws.begin ();
                const_watch_iterator k;
                bool keep_i = false;
                for (k = j; k != end; k++) {
                    Watch w = *k;
                    Clause *c = w.clause;

                    for (int alpha_c_j=0; alpha_c_j < neg_alpha_c_minus_c0.size();alpha_c_j++) {
                        if (c-> size == 2 && ((c->literals[0] == alpha_a[i] && c->literals[1] == -neg_alpha_c_minus_c0[alpha_c_j]) || (c->literals[1] == alpha_a[i] && c->literals[0] == -neg_alpha_c_minus_c0[alpha_c_j] ))) {
                            neg_alpha_c_minus_c0.erase(neg_alpha_c_minus_c0.begin() + alpha_c_j);
                            keep_i = true;
                            break;
                        }
                    }
                }

                if (keep_i) {
                    alpha_a_useful.push_back(alpha_a[i]);
                }
            }
        // otherwise try to shrink clauses using internal propagator
        } else {
            for (int i=0; i < alpha_a.size(); i++){
                printf("Trying to shrink using %d\n", alpha_a[i]);

                // want to skip over fixed literals using something like this
                // todo : I think this shouldn't be necessary because we checked it earlier when creating alpha_a
                Flags &f = flags (alpha_a[i]);

                if (f.status == Flags::FIXED) {
                    // printf("We are skipping %d \n", alpha_a[i]);
                    continue;
                }   

                // need to backtrack from existing state
                // printf("Before backtrack; propagated: %d; trail.size: %d \n", propagated, trail.size ());

                backtrack (0);
                // printf("right after a backtrack!\n");
                // print_assignment ();
                // printf("checking propagations from %d \n", -alpha_a[i]);
                // printf("We have propagated: %d; propagated: %d; trail.size: %d \n", -alpha_a[i], propagated, trail.size ());

                search_assume_decision(-alpha_a[i]); 

                // if we learn a singleton conflict, the gbc becomes trivial
                bool dont_learn_gbc = false;


                if (!propagate ()) {
                    printf("We got a conflict when propagating from %d\n", -alpha_a[i]);
                    // print_assignment ();
                    // printf("At position 1; propagated: %d; trail.size: %d \n", propagated, trail.size ());

                    analyze ();
                    // printf("At position 2; propagated: %d; trail.size: %d \n", propagated, trail.size ());

                    // print_assignment ();
                    // I am not sure if the conflict checking here is completely correct
                    if (!propagate ()) {
                        // printf("At position 3; propagated: %d; trail.size: %d \n", propagated, trail.size ());
                        printf ("got to a conflict \n");
                        STOP (global);
                        analyze ();
                        return false;
                    }
                    // printf("found unit : %d! \n", alpha_a[i]);
                    // // printf("Removing %d from alpha_a_useful from position since it is trivial %d \n", alpha_a_useful[useful_alpha_i], useful_alpha_i);
                    // alpha_a_useful.erase(alpha_a_useful.begin() + useful_alpha_i);
                    // // printf("Now we have alpha_a_useful: ");
                    // print_vector(alpha_a_useful);
                    continue;
                } 

                bool keep_i = false;

                LOG(neg_alpha_c_minus_c0, "We have neg_alpha_c_minus_c0:");

                // vector<int> new_neg_alpha_c_minus_c0;
                // vector<int>::iterator it = neg_alpha_c_minus_c0.begin();

                for (int j=0; j < neg_alpha_c_minus_c0.size();) {
                    int v = val (neg_alpha_c_minus_c0[j]);
                    if (v < 0) {
                        // print_assignment ();
                        printf("The literal %d in ~alpha_a implies literal %d in alpha_c by unit propagation \n", -alpha_a[i], -neg_alpha_c_minus_c0[j]);
                        // new_neg_alpha_c_minus_c0.push_back(neg_alpha_c_minus_c0[j]);
                        assert (j < neg_alpha_c_minus_c0.size());
                        neg_alpha_c_minus_c0.erase(neg_alpha_c_minus_c0.begin() + j);
                        keep_i = true;

                        // todo: currently have to add these binary clauses to make proof gor through. I don't like this though
                        // clause.push_back(alpha_a[i]);
                        // clause.push_back(-neg_alpha_c[j]);
                        // sort_vec_by_decision_level(&clause);
                        // // printf("We are adding the binary unit-prop clause:");
                        // print_vector(clause);
                        // if (clause.size () > 1)
                        //     Clause* c = new_learned_redundant_clause (1);
                        // else {
                        //     assign_original_unit (++clause_id, clause[0]);
                        // }
                        // clause.clear ();
                    } else {
                        j++;
                    }
                }
                // neg_alpha_c_minus_c0 = new_neg_alpha_c_minus_c0;

                // if (erase_i) {
                //     // printf("Removing %d from alpha_a_useful from position %d \n", alpha_a_useful[useful_alpha_i], useful_alpha_i);
                //     alpha_a_useful.erase(alpha_a_useful.begin() + useful_alpha_i);
                //     // (printf)("Now we have alpha_a_useful: ");
                //     print_vector(alpha_a_useful);
                // } else 
                //     useful_alpha_i += 1;
                if (keep_i)
                    alpha_a_useful.push_back(alpha_a[i]);
            }
        // was remembering literals without this backtrack
        backtrack (0);
        }
    }

    printf("\n AFTERWARDS WE HAVE:");
    printf("We have alpha_a:");
    print_vector(alpha_a);
    printf("\n");

    printf("We have alpha_a_useful:");
    print_vector(alpha_a_useful);
    printf("\n");

    printf("We have neg_alpha_c:");
    print_vector(neg_alpha_c);
    printf("\n");

    printf("We have neg_alpha_c_minus_c0:");
    print_vector(neg_alpha_c_minus_c0);
    printf("\n");

    minus_nineteen_var = var (-19);
    minus_nineteen_reason = minus_nineteen_var.reason;

    if (minus_nineteen_reason) {
        printf("3. The reason for -19 is: \n");
        print_clause(minus_nineteen_reason);
    }

    


    
    bool adding_a_clause = false;
    for (int i=0; i < min(opts.maxglobalblock, clauses_to_add.size()); i++){
        // only add a clause of size at least 2

        adding_a_clause = true;


        if (alpha_a_useful.empty() || opts.globalnoshrink) {

            // new_clause := neg_alpha_c + (som literal in alpha_a)
            vector<int> new_clause = clauses_to_add[i];

            if (new_clause.size() > 1) {
                int last_element = new_clause.back ();
                sort_vec_by_decision_level(&new_clause);
                bool is_clause_trivial = opts.globalfiltertriv && check_if_clause_trivial (new_clause);
                clause = new_clause;
                // todo : got rid of non-shrunk clause learning for rn
                printf("With is_clause-triv: %d, we are adding the globally blocked clause (non-shrunk):", is_clause_trivial);
                print_vector(clause);
                printf("\n");
                vector<int> neg_alpha_c_a(neg_alpha_c);
                neg_alpha_c_a.push_back(last_element);

                if (opts.globalrecord && !is_clause_trivial) {
                    outFile_pr << last_element << " ";
                    outFile_pr <<  " ";
                    for (int val : neg_alpha_c_a) {
                        outFile << val << " ";
                        outFile_pr << val << " ";
                    }
                    for (int val : alpha_a) {
                        outFile_pr << val << " ";
                    }
                    outFile << "\n";
                    outFile.flush();
                    outFile_pr << "\n";
                    outFile_pr.flush();
                }

                if (opts.globallearn) {
                    printf("Learning the original clause (no shrink)! \n ");
                    if (!is_clause_trivial)   
                        Clause* c = new_learned_weak_irredundant_global_clause (last_element, neg_alpha_c_a, alpha_a, 1);
                }

          

                clause.clear ();

            }
        } else { //(!alpha_a_useful.empty()) {
            // clause = neg_alpha_c_minus_c0.append(alpha_a);
            vector<int> neg_alpha_c_minus_c0_alpha_a(neg_alpha_c_minus_c0);
            neg_alpha_c_minus_c0_alpha_a.insert(neg_alpha_c_minus_c0_alpha_a.end(), alpha_a_useful.begin(), alpha_a_useful.end());
            printf("checking if this clauses is trivial");
            bool is_clause_trivial = opts.globalfiltertriv && check_if_clause_trivial (neg_alpha_c_minus_c0_alpha_a);
            clause = neg_alpha_c_minus_c0_alpha_a;
            sort_vec_by_decision_level(&clause);
            // printf("We are adding the reduced globally blocked clause:");
            // print_vector(clause);
            if (opts.globallearn) {
                if (clause.size () > 1) {
                    printf("actually learning the clause!");
                    // todo remove this only taking binary clause
                    if (!is_clause_trivial)
                        Clause* c = new_learned_weak_irredundant_global_clause (alpha_a_useful.back(), neg_alpha_c_minus_c0_alpha_a, alpha_a, 1);
                    // Clause* c = new_learned_redundant_clause (1);
                    printf("With is_clause_trivial %d, we are adding the globally blocked clause: ", is_clause_trivial);
                    print_vector(alpha_a_useful);
                    printf(" ");
                    print_vector(neg_alpha_c_minus_c0_alpha_a);
                    printf("\n");
                } else {
                    // todo : currently, I can only assign a unit at level 0
                    if (!opts.globalbcp) {
                        assign_original_unit_gbc (++clause_id, clause[0], alpha_a);
                        printf("we are adding the globally blocked unit: %d\n", clause[0]);
                    }
                }
            }
            // printf("made it past the learning! \n");
            if (opts.globalrecord && !is_clause_trivial) {
                // printf("actually recording the clause!");
                    outFile_pr << alpha_a_useful.back() << " ";
                    for (int val : neg_alpha_c_minus_c0_alpha_a) {
                        outFile << val << " ";
                        outFile_pr << val << " ";
                    }
                    for (int val : alpha_a) {
                        outFile_pr << val << " ";
                    }
                    outFile << "\n";
                    outFile.flush();
                    outFile_pr << "\n";
                    outFile_pr.flush();
                }
            // printf("made it past the recording! \n");

            clause.clear ();
        }

        // note we do this inside the for loop, since we only want to do add one clause
        printf("exited globally blocked stuff \n");
        STOP (global);
        return adding_a_clause;
    }
    return false;
}

// bool Internal::globalling () {
//   LOG("in the globally blocked checking step \n");

//   if (!opts.global)
//     return false;
// //   if (!preprocessing && !opts.inprocessing)
// //     return false;
//   // for right now we only do global step in assumptions
// //   if (!in_assumptions)
// //     return false;
// //   if (preprocessing)
// //     assert (lim.preprocessing);

//   // Triggered in regular 'opts.globalint' conflict intervals.
//   //
// //   if (lim.global > stats.conflicts)
// //     return false;

//   if (level != 2) { //(0 == level || level > 5)
//     return false; // One decision necessary.
//   }

// //   if (global_switch) {
// //     return false;
// //   }

//   // printf("Checking the decisions %d and %d \n", global_decision1, global_decision2);

// //   // printf ("%d; %d;%d;%d;%d", abs (global_decision1), abs (global_decision2), max_var, 0 < abs (global_decision1) <= max_var, 0 < abs (global_decision2) <= max_var);

//   if (0 < abs (global_decision1) && abs (global_decision1) <= max_var && 0 < abs (global_decision2) && abs (global_decision2) <= max_var && is_decision (global_decision1) && is_decision (global_decision2))
//     return false;

//   // printf("made it past this check");


//   bool reached_first_decision = false;
// //   for (int i = 1; i <= Internal::max_var; i++) {
// //         const signed char tmp = val (i);
// //         if (tmp > 0 && is_decision (i)) {
// //             if (reached_first_decision) {
// //                 global_decision2 = i;
// //             } else  {
// //                 global_decision1 = i;
// //                 reached_first_decision = true;
// //             }
// //         } else if (tmp < 0 && is_decision (-i)) {
// //             if (reached_first_decision)
// //                 global_decision2 = -i;
// //             else {
// //                 global_decision1 = -i;
// //                 reached_first_decision = true;
// //             }
// //         }
// //     }

// for (int i =1; i< control.size (); i++) {
//     if (reached_first_decision) {
//         global_decision2 = control[i].decision;
//     } else {
//         global_decision1 = control[i].decision;
//         reached_first_decision = true;
//     }
// }

    // print_assignment ();

    // printf("Added new decisions %d and %d \n", global_decision1, global_decision2);


//   global_switch = true;

//   global_counter = global_counter + 1;

//   // runtime for  
//   if (global_counter % global_interval != 0) {
//     return false;
//   }

//   global_interval = (rand() % 8) + 2;

//   global_counter = global_counter + 1;


//   if (level <= averages.current.jump)
//     return false; // Main heuristic.

  // printf("Globalling with assignment");
//   print_assignment ();

//   return true;

//   if (!stats.current.irredundant)
//     return false;
//   double remain = active ();
//   if (!remain)
//     return false;
//   double ratio = stats.current.irredundant / remain;
//   return ratio <= opts.globalmaxrat;
// }








// // a version of globalling that also makes decision
// // todo: write this
// bool Internal::globalling_decide () {
//   LOG("in the globally blocked checking step \n");

//   if (!opts.global)
//     return false;
//   if (!preprocessing) // && !opts.inprocessing)
//     return false;
// //   // for right now we only do global step in assumptions
// //   if (!in_assumptions)
// //     return false;
//   if (preprocessing)
//     assert (lim.preprocessing);

//   // Triggered in regular 'opts.globalint' conflict intervals.
//   //
// //   if (lim.global > stats.conflicts)
// //     return false;

//   if (level != 0) { //(0 == level || level > 5)
//     return false; 
//     // printf("We are failing as we are in level %d", level);
//   }


//   // decide and propagate

//   vector<int> current_assignment;

//   // right now it is two random decisions
//   // try to figure out a better way to do it

// //   // printf("started the test thingy \n");
// //   print_assignment ();
// //   search_assume_decision(-1);
// //   if (!propagate ()) {
// //     // printf("got a conflict!\n");
// //     analyze ();
// //   }

// // search_assume_decision(527);


// //   if (!propagate ()) {
// //     // printf("got a conflict!\n");
// //     analyze ();
// //   }

// //    // printf("DOING A GLOBAL CHECK!!! \n");
// //    global_counter = global_counter + 1;
// //    least_conditional_part ();
// //    backtrack ();
// //    // printf("did the test thingy");

//    std::ofstream outFile("global_clauses.txt");
//    std::ofstream outFile_pr("global_clauses_pr.txt");

//     if (!outFile) {
//         error ("Error: File could not be created.");
//     }

//   if (!opts.globalrandom){
//     for (int i = 1; i <= Internal::max_var; i++) {
//         for (int j = 1; j <= Internal::max_var; j++) {
//         for (int sign = 0; sign < 4; sign++) {
//             if (j == i)
//                 continue;

//             int fst_sign = pow(-1, (sign/2));
//             int snd_sign = pow(-1, (sign % 2));

//             int fst_val = fst_sign * i;
//             int snd_val = snd_sign * j;

            

//             // printf("making the decisions %d and %d \n", fst_val, snd_val);
//             if (val (fst_val))
//                 continue;

//             // printf("    deciding first val \n");
//             search_assume_decision(fst_val);

//             if (!propagate ()) {
//                 // analyze ();
//                 // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
//                 backtrack ();
//                 continue;
//             }

//             if (val (snd_val)) {
//                 backtrack ();
//                 continue;
//             }
            
//             // printf("    deciding second val \n");
//             search_assume_decision(snd_val);
//             // printf("    made second search_assume_decision");

//             if (!propagate ()) {
//                 // analyze ();
//                 // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
//                 backtrack ();
//                 continue;
//             }

//             // printf("DOING A GLOBAL CHECK!!! \n");
//             global_counter = global_counter + 1;
//             least_conditional_part (outFile, outFile_pr);
//             backtrack ();
//         }
//         }
//      }
//   } else {
//     // printf("enters random stage");
//     for (int k = 0; k < 4 * Internal::max_var; k++) {
//         // printf("enters for loop");
//         // srand(std::time(0));
//         int i = (rand() % Internal::max_var) + 1;
//         // printf("one \n");
//         // srand(std::time(0));
//         // printf("two \n");
//         int j = (rand() % Internal::max_var) + 1;
//         // printf("We have i %d adn j %d with max_var %d \n", i, j , Internal::max_var);
//         if (i == j)
//             continue;


//         // srand(std::time(0));
//         // printf("three \n");
//         int sign = rand() % 4;

//         int fst_sign = pow(-1, (sign/2));
//         int snd_sign = pow(-1, (sign % 2));

//         int fst_val = fst_sign * i;
//         int snd_val = snd_sign * j;


//         // printf("making the decisions (randomly) %d and %d \n", fst_val, snd_val);
//         if (val (fst_val))
//             continue;

//         // printf("    deciding first val \n");
//         search_assume_decision(fst_val);

//         if (!propagate ()) {
//             // analyze ();
//             // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
//             backtrack ();
//             continue;
//         }

//         if (val (snd_val)) {
//             backtrack ();
//             continue;
//         }
        
//         // printf("    deciding second val \n");
//         search_assume_decision(snd_val);
//         // printf("    made second search_assume_decision");

//         if (!propagate ()) {
//             // analyze ();
//             // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
//             backtrack ();
//             continue;
//         }

//         // printf("DOING A GLOBAL CHECK!!! \n");
//         global_counter = global_counter + 1;
//         least_conditional_part (outFile, outFile_pr);
//         backtrack ();
//     }
//   }
// //   decide ();
// //   propagate ();

// //   decide ();
// //   propagate ();

  





//   // runtime for  
// //   if (global_counter % 8 != 0) {
// //     return false;
// //   }

// //   global_counter = global_counter + 1;


// //   if (level <= averages.current.jump)
// //     return false; // Main heuristic.

//   // made this false for right now
//   return false;

// //   if (!stats.current.irredundant)
// //     return false;
// //   double remain = active ();
// //   if (!remain)
// //     return false;
// //   double ratio = stats.current.irredundant / remain;
// //   return ratio <= opts.globalmaxrat;
// }

} // namespace CaDiCaL
