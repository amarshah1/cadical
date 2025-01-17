#include "internal.hpp"
#include <map>
#include <fstream>  // For file handling
using namespace std;

namespace CaDiCaL {


void Internal::print_clause (CaDiCaL::Clause *const &c) {
    // for(const_literal_iterator l = c->begin (); l != c->end (); l++){
    //         const int lit = *l;
    //         printf("%d ", lit);
    //     }
}

// kind've silly rat detection that will work for our case
bool Internal::direction() {

    START (direction);


    int assumption = assumptions[0];

    LOG("checking the DIRECTION of %d", assumption);

    LOG("We are checking if %d occurs only positively or negatively", assumption);

    bool occurs_positively = false;
    bool occurs_negatively = false;

    for (const auto &c: clauses) { 


        for(const_literal_iterator l = c->begin (); l != c->end (); l++){
            if (*l == assumption) {
                occurs_positively = true;
                LOG("occurs positively at:");
                print_clause (c);
            }
            else if (*l == -assumption) {
                occurs_negatively = true;
                LOG("occurs negatively at:");
                print_clause (c);
            }
            if (occurs_negatively && occurs_positively) {
                break;
            }
        }
    }

    if (occurs_positively && !occurs_negatively) {
        learn_unit_clause (assumption);
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
                eager_delete_clause (c);
            
        }
    } else if (!occurs_positively && occurs_negatively) {
        learn_unit_clause (-assumption);
        for (const auto &c: clauses) { 
            bool occurs_in_clause = false;
            vector<int> other_lits = {};

            LOG("looking at clause:");
            print_clause (c);

            for(const_literal_iterator l = c->begin (); l != c->end (); l++){
                if (*l == -assumption) {
                    LOG ("occurs in clause");
                    occurs_in_clause = true;
                }
            }

            if (occurs_in_clause)
                eager_delete_clause (c);
            
        }
    }


    STOP (direction);
    
    return occurs_positively ^ occurs_negatively;

}

bool Internal::directioning() {
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
