// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
using namespace std;

#include "auxlib.h"
#include "lyutils.h"
#include "astree.h"



struct symbol {
    attr_bitset attributes;
    unordered_map<const string*,symbol*>* fields;
    size_t filenr, linenr, offset, block_nr;
    vector<symbol*>* parameters;
};

using symbol_table = unordered_map<const string*,symbol*>;
using symbol_entry = pair<const string*,symbol*>;

void postorder (astree* tree);


#endif
