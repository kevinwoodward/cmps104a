// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
using namespace std;

#include "auxlib.h"

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

   // Functions.
   astree (int symbol, const location&, const char* lexinfo);
   ~astree();
   astree* adopt (astree* child1, astree* child2 = nullptr);
   astree* synthesize_prototype (int symbol,
                                        astree* identdecl,
                                        astree* func_params);
   static astree* synthesize_root(astree* new_root);
    astree* synthesize_function (int symbol,
                                        astree* identdecl,
                                        astree* func_params,
                                        astree* block);
   astree* adopt_sym (astree* child, int symbol);
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

#endif
