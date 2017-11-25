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

void postorder (astree* tree)
{
   assert (tree != nullptr);

   for (size_t child = 0; child < tree->children.size(); ++child) {
      postorder(tree->children.at(child));
   }
   printf("\n%d\n", tree->symbol);
   // Call switch func to set attr + typecheck
}

symbol* table_find_var(symbol_table* table, astree* node)
{
    if ((table->count (node->lexinfo) == 0) || (table->empty())) { //TODO: is the table-> empty case really needed?
        return nullptr;
    }
    return (table->find (node->lexinfo))->second;
}


symbol* stack_find_var(astree* node)
{
    for(auto table : symbol_stack) //TODO: will this go from most local to global scope??
    {
        if(table != nullptr)
        {
            if(!(table->empty()))
            {
                if (table_find_var(table, node) != nullptr)
                {
                    return table_find_var(table, node);
                }
            }
        }
    }
    return nullptr;
}


symbol* create_symbol (astree* node)
{
    symbol* sym = new symbol;
    sym->attributes = node->attributes;
    sym->fields = nullptr; //TODO: is this correct?
    sym->filenr = node->lloc.filenr;
    sym->linenr = node->lloc.linenr;
    sym->offset = node->lloc.offset; //OFFSET WOO WOO WOO WOO WOO
    sym->block_nr = node->block_nr;
    sym->parameters = nullptr; //TODO: is this correct?

    return sym;
}

void adopt_attrs (astree* parent, astree* child)
{
    for (int i = 0; i < ATTR_bitset_size; i++) {
        if (child->attributes[i] == 1) {
            parent->attributes.set(i);
        }
    }
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

    symbol* temp_symbol;

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
            // Get attributes from symtab stack
            temp_symbol = stack_find_var(node);
            //TODO: if temp_symbol is null then check types
                //if null after that, throw error: "not found"
            node->attributes = temp_symbol->attributes;
            break;
        case TOK_TYPEID:
            node->attributes.set(ATTR_typeid);
            break;
        case TOK_FIELD:
            node->attributes.set (ATTR_field);
            //TODO: idk lol see the pdf table fig1
            break;
        case TOK_ARRAY:
            left->attributes.set(ATTR_array);
            if (left == nullptr || left->children.empty())
            {
                break;
            }
            left->children[0]->attributes.set (ATTR_array);
            break;
        case TOK_VOID:
            left->attributes.set(ATTR_void);
            break;
        case TOK_INT:
            if(left == nullptr)
            {
                break;
            }
            left->attributes.set(ATTR_int);
            //TODO: inherit type
            break;
        case TOK_STRING:
            if(left == nullptr)
            {
                break;
            }
            left->attributes.set(ATTR_string);
            //TODO: inherit type
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
            left->children[0]->attributes.set(ATTR_lval);
            left->children[0]->attributes.set(ATTR_variable);
            adopt_attrs(node, left); //Why is this needed?
            if(symbol_stack.back() == nullptr)
            {
                //create new symbol table and push onto stack
                //push symbol on newly created symbol table
                temp_symbol = create_symbol(node); //TODO: duplicate declaration because can't declare in switch stmt
                symbol_table* temp_table = new symbol_table();
                temp_table->insert (symbol_entry (node->lexinfo, temp_symbol)); //TODO: symbol_entry correct?
                symbol_stack.push_back(temp_table);
            }
            else
            {
                //push symbol on existing top symbol table
                temp_symbol = create_symbol(node); //TODO: duplicate declaration because can't declare in switch stmt
                (symbol_stack.back())->insert (symbol_entry (node->lexinfo, temp_symbol));
            }

            //look up in type names table
            //if struct do for loop
            break;
        case TOK_WHILE:
            //TODO: check condition?
            break;
        case TOK_IF:
            //TODO: check condition?
            break;
        case TOK_IFELSE: //TODO: changed from else. SHould be else or ifelse?
            //TODO: check condition?
            break;
        case TOK_RETURN:
            break;
        case TOK_RETURNVOID:
            break;
        case '=':
            if(left == nullptr)
            {
                break;
            }
            //TODO inherit type, typecheck operands
            break;
        case '+':
            //TODO: should be same as -
            break;
        case '-':
            //TODO: should be same as +
            break;
        case '*':
            //TODO: should be same as / and %
            break;
        case '/':
            //TODO: should be same as * and %
            break;
        case '%':
            //TODO: should be same as / and *
            break;
        case TOK_EQ: //TODO: i think nothing is needed for these comparison ops.
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
            //TODO: not necessary i think
            break;
        case TOK_NEG:
            //TODO: not necessary i think
            break;
        case '!':
            node->attributes.set (ATTR_vreg);
            //TODO: set attribute for type int? since there is no bool
            //TODO: check type of operand
            break;
        case TOK_NEW:
            //TODO: inherit attribs
            break;
        case TOK_NEWSTRING:
            node->attributes.set (ATTR_vreg);
            node->attributes.set (ATTR_string);
            break;
        case TOK_NEWARRAY:
            node->attributes.set (ATTR_vreg);
            node->attributes.set (ATTR_array);
            //TODO: inherit type
            break;
        case TOK_CALL:
            //TODO: all of it.
            break;
        case TOK_INDEX:
            node->attributes.set (ATTR_lval);
            node->attributes.set (ATTR_vaddr);
            break;
        case TOK_INTCON:
            node->attributes.set (ATTR_int);
            node->attributes.set (ATTR_const);
            break;
        case TOK_CHARCON:
            node->attributes.set (ATTR_int); //no char attr
            node->attributes.set (ATTR_const);
            break;
        case TOK_STRINGCON:
            node->attributes.set (ATTR_string);
            node->attributes.set (ATTR_const);
            break;
        case TOK_NULL:
            node->attributes.set (ATTR_null);
            node->attributes.set (ATTR_const);
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
