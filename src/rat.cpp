#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
using namespace std;

namespace CaDiCaL {


// void Internal::print_clause (CaDiCaL::Clause *const &c) {
//     // for(const_literal_iterator l = c->begin (); l != c->end (); l++){
//     //         const int lit = *l;
//     //         printf("%d ", lit);
//     //     }
// }

// kind've silly rat detection that will work for our case
bool Internal::detect_rat() {

    START (rat);


    int assumption = -assumptions[0];

    LOG("We are checking if %d is a RAT", assumption);

    bool is_a_rat = true;

    for (const auto &c: clauses) { 


        bool occurs_in_clause = false;
        vector<int> other_lits = {};

        for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            if (*l == -assumption) {
                occurs_in_clause = true;
            }
            other_lits.push_back (*l);
        }


        if (!occurs_in_clause)
            continue;

        LOG("IT OCCURS IN CLAUSE: ");
        print_clause (c);


        // todo right now will only consider if 
        if (other_lits.size() != 1) {
            is_a_rat = false;
            break;
        }

        bool occurs_negatively = false;
        for (const auto &c_prime: clauses) { 
            if (clause.size() == 1 && clause[0] == -other_lits[0]) {
                occurs_negatively = true;
            }
        }
        if (!occurs_negatively) {
            is_a_rat = false;
            break;
        }
    }

    LOG("We have come to the conclusion that %d is a RAT: %d", assumption, is_a_rat);

    if (is_a_rat) {
        for (const auto &c: clauses) { 


            bool occurs_in_clause = false;
            vector<int> other_lits = {};

            LOG("looking at clause:");
            print_clause (c);

            for(const_literal_iterator l = c->begin (); l != c->end (); l++){
                if (*l == assumption) {
                    LOG ("occurs in clause");
                    occurs_in_clause = true;
                }
            }

            if (occurs_in_clause)
                delete_clause (c);
            
        }
    }
    STOP (rat);
    return is_a_rat;

}

bool Internal::rat_finding () {
  if (!in_assumptions) {
    return false;
  }

  if (level > 1) {
    return false;
  }

  if (assumptions.size() != 1)
    return false;

  if (assumptions[0] > 0)
    return false;

  return true;
}


    

} // namespace CaDiCaL
