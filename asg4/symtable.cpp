// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <unordered_map>
#include <utility>
#include "symtable.h"
#include "astree.h"

using namespace std;

symbol_table* struct_table = new symbol_table();
vector<symbol_table*> symbol_stack;
int next_block = 0;

void postorder (astree* tree) {
   assert (tree != nullptr);

   for (size_t child = 0; child < tree->children.size(); ++child) {
      postorder(tree->children.at(child));
   }
   printf("\n%d\n", tree->symbol);
   // Call switch func to set attr + typecheck
}

void insert_into_struct_table (symbol_table* table, astree* node)
{
    symbol_table* field_table = new symbol_table();
    for (auto child : node->children) {
        if(child == node->children[0])
            continue;
        child->attributes.set(ATTR_field); //delete maybe. Maybe deleted.
        symbol* child_symbol = new symbol;
        child_symbol->attributes = child->attributes;
        child_symbol->fields = nullptr;
        child_symbol->filenr = child->lloc.filenr;
        child_symbol->linenr = child->lloc.linenr;
        child_symbol->offset = child->lloc.offset; //OFFSET WOO WOO WOO WOO WOO
        child_symbol->block_nr = child->block_nr;
        child_symbol->parameters = nullptr;

        field_table->insert (symbol_entry (child->lexinfo, child_symbol));
    }
    //create new field table


    symbol* node_symbol = new symbol;
    node_symbol->attributes = node->attributes;
    node_symbol->fields = field_table;
    node_symbol->filenr = node->lloc.filenr;
    node_symbol->linenr = node->lloc.linenr;
    node_symbol->offset = node->lloc.offset; //OFFSET WOO WOO WOO WOO
    node_symbol->block_nr = node->block_nr;
    node_symbol->parameters = nullptr;

    table->insert (symbol_entry (node->children[0]->lexinfo, node_symbol));
}


/*
    TOK_FUNCTION TOK_PROTOTYPE
    TOK_WHILE TOK_ARRAY
    TOK_STRUCT TOK_IDENT TOK_ARRAY
    TOK_VOID TOK_INT TOK_STRING TOK_IDENT
    TOK_BLOCK TOK_VARDECL TOK_WHILE TOK_IF
    TOK_ELSE TOK_RETURN TOK_EQ/NE/LE/GE/LT/GT
    TOK_POS TOK_NEG TOK_NEW TOK_CALL TOK_INDEX
    TOK_FIELD TOK_INTCON TOK_CHARCON TOK_STRINGCON
    TOK_NULL + - / % . !
*/
void set_attributes (astree* node)
{
    astree* left;
    astree* middle;
    astree* right;
    //symbol* symbol;
    if (node->children.size() > 0) {
        left = node->children[0];
    }
    if (node->children.size() > 1) {
        right = node->children[1];
    }
    if(node->children.size() == 3) {
        middle = right;
        right = node->children[2];
    }

    switch (node->symbol) {
        case TOK_ROOT:
            break;
        case TOK_STRUCT:
            // insert into type name table
            // insert into struct table with all other defs
            left->attributes.set(ATTR_typeid);
            for (auto child : node->children) {
                if(child == node->children[0])
                    continue;
                child->attributes.set(ATTR_field);
            }
            insert_into_struct_table(struct_table, node);

            break;
        case TOK_IDENT:
            break;
        case TOK_TYPEID:
            break;
        case TOK_FIELD:
            break;
        case TOK_ARRAY:
            break;
        case TOK_VOID:
            break;
        case TOK_INT:
            break;
        case TOK_STRING:
            break;
        case TOK_FUNCTION:
            // enter new block
            // func
            //      name = left
            //      paramlist = middle '('
            //      block = right
            // set each identdecl in
            // paramlist to ATTR_param
            // and ATTR_lval
            left->attributes.set(ATTR_function);
            for (auto child : middle->children) {
                child->attributes.set(ATTR_variable);
                child->attributes.set(ATTR_param);
                child->attributes.set(ATTR_lval); // page 3, 2.2(d)
            }
            break;
        case TOK_PROTOTYPE:
            break;
        case TOK_PARAMLIST:
            break;
        case TOK_DECLID:
            break;
        case TOK_BLOCK:
            symbol_stack.push_back(nullptr);
            next_block += 1;
            break;
        case TOK_VARDECL:
            // enter into identifier symbol table1
            if(symbol_stack.back() == nullptr)
            {
                //create new symbol table and push onto stack
                //push symbol on newly created symbol table
            }
            else
            {
                //push symbol on newly created symbol table
            }

            //look up in type names table1//
            //if struct do for loop
            break;
        case TOK_WHILE:
            break;
        case TOK_IF:
            break;
        case TOK_ELSE:
            break;
        case TOK_RETURN:
            break;
        case TOK_RETURNVOID:
            break;
        case '=':
            break;
        case '+':
            break;
        case '-':
            break;
        case '*':
            break;
        case '/':
            break;
        case '%':
            break;
        case TOK_EQ:
            break;
        case TOK_NE:
            break;
        case TOK_LE:
            break;
        case TOK_GE:
            break;
        case TOK_LT:
            break;
        case TOK_GT:
            break;
        case TOK_POS:
            break;
        case TOK_NEG:
            break;
        case '!':
            break;
        case TOK_NEW:
            break;
        case TOK_NEWSTRING:
            break;
        case TOK_NEWARRAY:
            break;
        case TOK_CALL:
            break;
        case TOK_INDEX:
            break;
        case TOK_INTCON:
            break;
        case TOK_CHARCON:
            break;
        case TOK_STRINGCON:
            break;
        case TOK_NULL:
            break;
    }
}

void set_attrib_to_children(astree* node) {

}

/*
Here we need code to do a traversal
through the astree and set the different attributes
this is called after the parser.y returns the tree
to main, then we call this with the tree and outfile



*/
