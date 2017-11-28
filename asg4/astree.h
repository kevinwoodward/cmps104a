// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <bitset>
using namespace std;

#include "auxlib.h"

enum { ATTR_void, ATTR_int, ATTR_null, ATTR_string, ATTR_struct,
ATTR_array, ATTR_function, ATTR_variable, ATTR_field, ATTR_typeid,
ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg,ATTR_vaddr,
ATTR_bitset_size };
using attr_bitset = bitset<ATTR_bitset_size>;

struct location {
   size_t filenr;
   size_t linenr;
   size_t offset;
};


struct astree {

   // Fields.
   int symbol;               // token code
   location lloc;            // source location
   const string* lexinfo;    // pointer to lexical information
   vector<astree*> children; // children of this n-way node
   attr_bitset attributes;
   int block_nr = 0;

   // Functions.
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr);

   static astree* synthesize_root(astree* new_root);
   static astree* synthesize_function (astree* identdecl,
                                       astree* func_params,
                                       astree* block);

   astree* adopt_sym (int symbol_,
                      astree* child1,
                      astree* child2 = nullptr);
   astree* adopt_with_sym (int symbol_,
                           astree* child1,
                           astree* child2 = nullptr);
   astree* adopt_ifelse (astree* predicate,
                         astree* then_stmt,
                         astree* else_stmt = nullptr);
   void dump_node (FILE*);
   void dump_tree (FILE*, int depth = 0);
   static void dump (FILE* outfile, astree* tree);
   static void print (FILE* outfile, astree* tree, int depth = 0);
};


void destroy (astree* tree1,
            astree* tree2 = nullptr,
            astree* tree3 = nullptr);

void errllocprintf (const location&, const char* format, const char*);

char* get_attributes (attr_bitset attributes);

#endif
