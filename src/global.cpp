#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
#include <cstdlib>
#include <ctime>
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
    printf("\n");
}

void Internal::print_clause (CaDiCaL::Clause *const &c) {
    for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            const int lit = *l;
            printf("%d ", lit);
        }
}

void Internal::print_vector (vector<int> c) {
    for(int i = 0; i < c.size(); i++){
            printf("%d ", c[i]);
        }
    printf("\n");
}

void Internal::print_all_clauses() {
    // printf ("Printing all clauses: \n");
    // for (const auto &c: clauses) {
    //     printf( "    ");
    //     print_clause (c);
    //     printf("\n");
    // }
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

bool Internal::least_conditional_part(std::ofstream& outFile) {

    START (global);

    printf("we have propagated: %d \n", propagated);
    printf("we have trail.size: %d \n", trail.size ());
    print_assignment();
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
            printf("skipping clause: ");
            print_clause (c);
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
            printf("skipping clause2: ");
            print_clause (c);
            continue;
        }

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
                // print_clause (c);
                // add to touched list if not already in neg_alpha_c
                if (!getbit(lit, 0)) {
                    alpha_touches.push_back(lit);
                    // printf("    length of alpha_touches is: %d \n", alpha_touches.size());
                }
            }
        }

        // LOG("\n    touched wihtout sat is: %d", touched_without_sat);
        if (!satisfies_clause) {
            LOG("    we do not satisfy the clause. Alpha_touches size: %d \n", alpha_touches.size());
            // add alpha_touches to neg_alpha_c
            neg_alpha_c.insert(neg_alpha_c.end(), alpha_touches.begin(), alpha_touches.end());
            // set "added to neg_alpha_c" bit
            for (int i=0; i < alpha_touches.size(); i++){
                printf("adding to neg_alpha_c: %d \n", alpha_touches[i]);
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
                // printf("[AUTARKY DECISION ALERT: %d]", key);
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
    // printf("\n");

    // if (print_out) {
    //     int alpha_c_decision = 0;

    //     for (int i=0; i < neg_alpha_c.size(); i++) {
    //         if (is_decision (neg_alpha_c[i]))
    //             alpha_c_decision++;
    //     }

    //     int alpha_a_decision = 0;

    //     if (max_key && is_decision (max_key)) {
    //         alpha_a_decision += 1;
    //     }

    //     for (int i=0; i < alpha_a.size(); i++) {
    //         if (is_decision (alpha_a[i]))
    //             alpha_a_decision++;
    //     }


    //     std::ofstream file("tmp2.txt", std::ios::app);  // Open file in append mode

    //     // Check if the file opened successfully
    //     if (!file) {
    //         LOG("Error opening file for writing!");
    //         return false;
    //     }
    //     else {

    //         // Write x and y in "x, y" format
    //         file << neg_alpha_c.size() << ", " << alpha_a.size() << "," << max_key << ", " << (length_of_current_assignment ()) << ", " << alpha_c_decision << ", " << alpha_a_decision << ", " << level << std::endl;

    //         file.close();  // Close the file
    //     }
    //     }

    // have to unset all of the bits
    for(int i=0; i < neg_alpha_c.size(); i++){
        unsetbit(neg_alpha_c[i], 0);
    }


    // it is unsound add globally blocked clause when neg_alpha_c = alpha
    // if (max_key == 0) {
    //     STOP (global);
    //     printf("We are exiting: %d\n", clauses_to_add.size());
    //     return false;
    // }


    vector <int>neg_alpha_c_minus_c0(neg_alpha_c);
    vector <int>alpha_a_useful(alpha_a);

    int useful_alpha_i = 0;

    LOG(neg_alpha_c_minus_c0, "We start with neg_alpha_c_minus_c0:");
    LOG(alpha_a, "We start with alpha_a:");
    // for (int i =0; i < alpha_a.size (); i++) {
    //     if (is_decision (alpha_a[i])) {
    //         LOG("is decision: %d", i);
    //     }
    // }

    printf("We are at level: %d \n", level);

    print_assignment ();

    printf("We have alpha_a before:");
    print_vector(alpha_a);

    printf("We have neg_alpha_c before:");
    print_vector(neg_alpha_c);



    for (int i=0; i < alpha_a.size(); i++){

        // want to skip over fixed literals using something like this
        // todo : I think this shouldn't be necessary because we checked it earlier when creating alpha_a
        Flags &f = flags (alpha_a[i]);

        if (f.status == Flags::FIXED) {
            printf("We are skipping %d \n", alpha_a[i]);
            continue;
        }   

        // need to backtrack from existing state
        backtrack (0);
        print_assignment ();
        printf("checking propagations from %d \n", -alpha_a[i]);
        printf("We have propagated: %d; trail.size: %d \n", -alpha_a[i], propagated, trail.size ());

        search_assume_decision(-alpha_a[i]); 


        if (!propagate ()) {
            printf("We got a conflict when propagating from %d\n", -alpha_a[i]);
            print_assignment ();
            analyze ();
            propagate ();
            print_assignment ();
            if (unsat) {
                STOP (global);
                return false;
            }
            // todo : this needs to be a loop
            printf("found unit : %d! \n", alpha_a[i]);
            useful_alpha_i += 1;
            continue;
        } 

        bool erase_i = true;

        LOG(neg_alpha_c_minus_c0, "We have neg_alpha_c_minus_c0:");


        for (int j=0; j < neg_alpha_c_minus_c0.size();) {
            int v = val (neg_alpha_c_minus_c0[j]);
            if (v < 0) {
                printf("The literal %d in ~alpha_a implies literal %d in alpha_c by unit propagation \n", -alpha_a[i], -neg_alpha_c_minus_c0[j]);
                neg_alpha_c_minus_c0.erase(neg_alpha_c_minus_c0.begin() + j);
                erase_i = false;

                // todo: currently have to add these binary clauses to make proof gor through. I don't like this though
                // clause.push_back(alpha_a[i]);
                // clause.push_back(-neg_alpha_c[j]);
                // sort_vec_by_decision_level(&clause);
                // printf("We are adding the binary unit-prop clause:");
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

        if (erase_i) {
            printf("Removing %d from alpha_a_useful from position %d \n", alpha_a_useful[useful_alpha_i], useful_alpha_i);
            alpha_a_useful.erase(alpha_a_useful.begin() + useful_alpha_i);
            printf("Now we have alpha_a_useful: ");
            print_vector(alpha_a_useful);
        } else 
            useful_alpha_i += 1;
    }


    printf("We have alpha_a:");
    print_vector(alpha_a);

    printf("We have alpha_a_useful:");
    print_vector(alpha_a_useful);

    printf("We have neg_alpha_c:");
    print_vector(neg_alpha_c);

    printf("We have neg_alpha_c_minus_c0:");
    print_vector(neg_alpha_c_minus_c0);


    // was remembering literals without this backtrack
    backtrack (0);
    bool adding_a_clause = false;
    for (int i=0; i < min(opts.maxglobalblock, clauses_to_add.size()); i++){
        // only add a clause of size at least 2

        adding_a_clause = true;


        if (alpha_a_useful.empty()) {

            // new_clause := neg_alpha_c + (som literal in alpha_a)
            vector<int> new_clause = clauses_to_add[i];

            if (new_clause.size() > 1) {
                int last_element = new_clause.back ();
                sort_vec_by_decision_level(&new_clause);
                clause = new_clause;
                printf("we are adding the globally blocked clause:");
                print_vector(clause);
                vector<int> neg_alpha_c_a(neg_alpha_c);
                neg_alpha_c_a.push_back(last_element);
                if (opts.globallearn) {
                    Clause* c = new_learned_weak_irredundant_global_clause (last_element, neg_alpha_c_a, alpha_a, 1);
                }
                if (opts.globalrecord) {
                    for (int val : neg_alpha_c_a) {
                        outFile << val << " ";
                    }
                    outFile << "\n";
                    outFile.flush();
                }
                clause.clear ();
            }
        } else { //(!alpha_a_useful.empty()) {
            // clause = neg_alpha_c_minus_c0.append(alpha_a);
            neg_alpha_c_minus_c0.insert(neg_alpha_c_minus_c0.end(), alpha_a_useful.begin(), alpha_a_useful.end());
            clause = neg_alpha_c_minus_c0;
            sort_vec_by_decision_level(&clause);
            printf("We are adding the reduced globally blocked clause:");
            print_vector(clause);
            if (opts.globallearn) {
                if (clause.size () > 1) {
                    printf("actually learning the cluase!");
                    Clause* c = new_learned_weak_irredundant_global_clause (alpha_a_useful.back(), neg_alpha_c_minus_c0, alpha_a, 1);
                    // Clause* c = new_learned_redundant_clause (1);
                } else {
                    assign_original_unit (++clause_id, clause[0]);
                }
            }
            printf("made it past the learning! \n");
            if (opts.globalrecord) {
                printf("actually recording the clause!");
                    for (int val : neg_alpha_c_minus_c0) {
                        outFile << val << " ";
                    }
                    outFile << "\n";
                    outFile.flush();
                }
            printf("made it past the recording! \n");

            clause.clear ();
        }

    // note we do this inside the for loop, since we only want to do add one clause
    STOP (global);
    return adding_a_clause;
    }
    return false;
}

bool Internal::globalling () {
  LOG("in the globally blocked checking step \n");

  if (!opts.global)
    return false;
//   if (!preprocessing && !opts.inprocessing)
//     return false;
  // for right now we only do global step in assumptions
//   if (!in_assumptions)
//     return false;
//   if (preprocessing)
//     assert (lim.preprocessing);

  // Triggered in regular 'opts.globalint' conflict intervals.
  //
//   if (lim.global > stats.conflicts)
//     return false;

  if (level != 2) { //(0 == level || level > 5)
    return false; // One decision necessary.
  }

//   if (global_switch) {
//     return false;
//   }

  printf("Checking the decisions %d and %d \n", global_decision1, global_decision2);

  if (global_decision1 <= max_var && global_decision2 <= max_var && is_decision (global_decision1) && is_decision (global_decision2))
    return false;


  bool reached_first_decision = false;
//   for (int i = 1; i <= Internal::max_var; i++) {
//         const signed char tmp = val (i);
//         if (tmp > 0 && is_decision (i)) {
//             if (reached_first_decision) {
//                 global_decision2 = i;
//             } else  {
//                 global_decision1 = i;
//                 reached_first_decision = true;
//             }
//         } else if (tmp < 0 && is_decision (-i)) {
//             if (reached_first_decision)
//                 global_decision2 = -i;
//             else {
//                 global_decision1 = -i;
//                 reached_first_decision = true;
//             }
//         }
//     }

for (int i =1; i< control.size (); i++) {
    if (reached_first_decision) {
        global_decision2 = control[i].decision;
    } else {
        global_decision1 = control[i].decision;
        reached_first_decision = true;
    }
}

    print_assignment ();

    printf("Added new decisions %d and %d \n", global_decision1, global_decision2);


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

  printf("Globalling with assignment");
  print_assignment ();

  return true;

//   if (!stats.current.irredundant)
//     return false;
//   double remain = active ();
//   if (!remain)
//     return false;
//   double ratio = stats.current.irredundant / remain;
//   return ratio <= opts.globalmaxrat;
}








// a version of globalling that also makes decision
// todo: write this
bool Internal::globalling_decide () {
  LOG("in the globally blocked checking step \n");

  if (!opts.global)
    return false;
  if (!preprocessing) // && !opts.inprocessing)
    return false;
//   // for right now we only do global step in assumptions
//   if (!in_assumptions)
//     return false;
  if (preprocessing)
    assert (lim.preprocessing);

  // Triggered in regular 'opts.globalint' conflict intervals.
  //
//   if (lim.global > stats.conflicts)
//     return false;

  if (level != 0) { //(0 == level || level > 5)
    return false; 
    printf("We are failing as we are in level %d", level);
  }


  // decide and propagate

  vector<int> current_assignment;

  // right now it is two random decisions
  // try to figure out a better way to do it

//   printf("started the test thingy \n");
//   print_assignment ();
//   search_assume_decision(-1);
//   if (!propagate ()) {
//     printf("got a conflict!\n");
//     analyze ();
//   }

// search_assume_decision(527);


//   if (!propagate ()) {
//     printf("got a conflict!\n");
//     analyze ();
//   }

//    printf("DOING A GLOBAL CHECK!!! \n");
//    global_counter = global_counter + 1;
//    least_conditional_part ();
//    backtrack ();
//    printf("did the test thingy");

   std::ofstream outFile("global_clauses.txt");
    if (!outFile) {
        error ("Error: File could not be created.");
    }

  if (!opts.globalrandom){
    for (int i = 1; i <= Internal::max_var; i++) {
        for (int j = 1; j <= Internal::max_var; j++) {
        for (int sign = 0; sign < 4; sign++) {
            if (j == i)
                continue;

            int fst_sign = pow(-1, (sign/2));
            int snd_sign = pow(-1, (sign % 2));

            int fst_val = fst_sign * i;
            int snd_val = snd_sign * j;

            

            printf("making the decisions %d and %d \n", fst_val, snd_val);
            if (val (fst_val))
                continue;

            printf("    deciding first val \n");
            search_assume_decision(fst_val);

            if (!propagate ()) {
                // analyze ();
                // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
                backtrack ();
                continue;
            }

            if (val (snd_val)) {
                backtrack ();
                continue;
            }
            
            printf("    deciding second val \n");
            search_assume_decision(snd_val);
            printf("    made second search_assume_decision");

            if (!propagate ()) {
                // analyze ();
                // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
                backtrack ();
                continue;
            }

            printf("DOING A GLOBAL CHECK!!! \n");
            global_counter = global_counter + 1;
            least_conditional_part (outFile);
            backtrack ();
        }
        }
     }
  } else {
    printf("enters random stage");
    for (int k = 0; k < 4 * Internal::max_var; k++) {
        printf("enters for loop");
        // srand(std::time(0));
        int i = (rand() % Internal::max_var) + 1;
        printf("one \n");
        // srand(std::time(0));
        printf("two \n");
        int j = (rand() % Internal::max_var) + 1;
        printf("We have i %d adn j %d with max_var %d \n", i, j , Internal::max_var);
        if (i == j)
            continue;


        // srand(std::time(0));
        printf("three \n");
        int sign = rand() % 4;

        int fst_sign = pow(-1, (sign/2));
        int snd_sign = pow(-1, (sign % 2));

        int fst_val = fst_sign * i;
        int snd_val = snd_sign * j;


        printf("making the decisions (randomly) %d and %d \n", fst_val, snd_val);
        if (val (fst_val))
            continue;

        printf("    deciding first val \n");
        search_assume_decision(fst_val);

        if (!propagate ()) {
            // analyze ();
            // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
            backtrack ();
            continue;
        }

        if (val (snd_val)) {
            backtrack ();
            continue;
        }
        
        printf("    deciding second val \n");
        search_assume_decision(snd_val);
        printf("    made second search_assume_decision");

        if (!propagate ()) {
            // analyze ();
            // seems like I do need a backtrack here, since analyze () will not necessarily backtrack to level 0 
            backtrack ();
            continue;
        }

        printf("DOING A GLOBAL CHECK!!! \n");
        global_counter = global_counter + 1;
        least_conditional_part (outFile);
        backtrack ();
    }
  }
//   decide ();
//   propagate ();

//   decide ();
//   propagate ();

  





  // runtime for  
//   if (global_counter % 8 != 0) {
//     return false;
//   }

//   global_counter = global_counter + 1;


//   if (level <= averages.current.jump)
//     return false; // Main heuristic.

  // made this false for right now
  return false;

//   if (!stats.current.irredundant)
//     return false;
//   double remain = active ();
//   if (!remain)
//     return false;
//   double ratio = stats.current.irredundant / remain;
//   return ratio <= opts.globalmaxrat;
}

    

} // namespace CaDiCaL
