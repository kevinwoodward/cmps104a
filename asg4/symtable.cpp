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
#include <iostream>

using namespace std;

// Definition: struct foo { int a;} <- in struct table
// Declaraction: foo a; <- in variable table

symbol_table* struct_table = new symbol_table();
symbol_table* type_table = new symbol_table();
vector<symbol_table*> symbol_stack;
int next_block = 0;

void new_block()
{
    next_block++;
    symbol_stack.push_back(nullptr);
}

void exit_block()
{
    symbol_stack.pop_back();
}

symbol* table_find_var(symbol_table* table, astree* node)
{
    auto symbol_iter = (table->find(node->lexinfo));
    if (symbol_iter == table->end())  //TODO: is the table-> empty case really needed?
    {
        return nullptr;
    }
    return (table->find (node->lexinfo))->second;
}



symbol* stack_find_var(astree* node)
{
    symbol_table* current_table = nullptr;
    for(int i = symbol_stack.size() - 1; i >= 0; i--)
    {
        current_table = symbol_stack[i];
        if(current_table != nullptr)
        {
            if(!(current_table->empty()))
            {

                return table_find_var(current_table, node);
            }
        }
    }
    return nullptr;
}

void blockTree_to_returnVec (astree* tree, vector<astree*>* &returnVec)
{
   if (tree == nullptr)
   {
        return;
   }

   for (size_t child = 0; child < tree->children.size(); ++child)
   {
      blockTree_to_returnVec(tree->children.at(child), returnVec);
   }
   //also chekc if return or returnvoid. but they wont have attributes yet

   //this works
   if(tree->symbol == TOK_RETURN || tree->symbol == TOK_RETURNVOID)
   {
       returnVec->push_back(tree);
   }
   return;

}

symbol* create_symbol (astree* node, vector<symbol*>* parameters = nullptr)
{
    symbol* sym = new symbol;
    sym->attributes = node->attributes;
    sym->fields = nullptr; //TODO: is this correct?
    sym->filenr = node->lloc.filenr;
    sym->linenr = node->lloc.linenr;
    sym->offset = node->lloc.offset; //OFFSET WOO WOO WOO WOO WOO
    sym->block_nr = node->block_nr;
    sym->parameters = parameters; //TODO: is this correct?

    return sym;
}
void insert_symbol (symbol_table* table,
                    astree* node,
                    vector<symbol*>* paramVec = nullptr) {
    symbol* symbol = create_symbol (node, paramVec);

    if ((table!=NULL) && (node!=NULL)) {
        table->insert (symbol_entry (node->lexinfo, symbol));
    }
}
vector<symbol*>* paramTree_to_symbolVec(astree* paramTree)
{
    vector<symbol*>* symbolVec = new std::vector<symbol*>();
    symbol* temp_symbol;
    for (auto child : paramTree->children)
    {
     temp_symbol = create_symbol(child);
     symbolVec->push_back(temp_symbol);

    }
    return symbolVec;


}

void define_ident (astree* node) {
    if (symbol_stack.back() == nullptr) {
        symbol_stack.back() = new symbol_table ();
    }
    insert_symbol (symbol_stack.back(), node);
}

void copy_attrs (astree* src, astree* dest)
{
    for (int i = 0; i < ATTR_bitset_size; i++) {
        if (src->attributes[i] == 1) {
            dest->attributes.set(i);
        }
    }
}
/*
    enum { ATTR_void, ATTR_int, ATTR_null, ATTR_string, ATTR_struct,
    ATTR_array, ATTR_function, ATTR_variable, ATTR_field, ATTR_typeid,
    ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr, ATTR_bitset_size };
*/



bool compatible_Atrribs(attr_bitset first, attr_bitset second)
{
    for(int i = 0; i < 7; i++)
    {
        if(first[i] && second[i]){
            return true;

        }
    }
    bool reference = second[ATTR_string]
                     || second[ATTR_array]
                     || second[ATTR_struct];
    if(first[ATTR_null] && reference)
    {
        return true;
    }
    return false;


}

bool compatible_Nodes(astree* first, astree* second)
{
   return compatible_Atrribs(first->attributes, second->attributes);

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
    astree* left = nullptr;
    astree* middle = nullptr;
    astree* right = nullptr;
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

    symbol* temp_symbol = nullptr;
    symbol* field_symbol = nullptr;
    vector<astree*>* return_Vec = nullptr;

    switch (node->symbol) {
        case TOK_ROOT:
            break;
        case TOK_STRUCT:
            //TODO: i think this whole logic is wrong
            //TODO: insert into type name table
            // insert into struct table with all other defs
            left->attributes.set(ATTR_typeid);
            for (auto child : node->children) {
                if(child == node->children[0])
                    continue;
                child->attributes.set(ATTR_field);
            }
            insert_into_struct_table(struct_table, node);
            //insert_into_type_table(type_table, node->children[0]);

            break;
        case TOK_IDENT:
            // Get attributes from symtab stack
            //TODO: if temp_symbol is null then check types
                //if null after that, throw error: "not found"


            temp_symbol = stack_find_var(node);
            if(temp_symbol != nullptr)
            {
                node->attributes = temp_symbol->attributes;
                cout << *node->lexinfo;
            }
            else if(node->attributes[ATTR_int] || node->attributes[ATTR_void])
            {
                printf("error\n"); //TODO: no print to stdout
            }
            break;
        case TOK_TYPEID:
            node->attributes.set(ATTR_typeid);
            break;
        case TOK_FIELD:
            node->attributes.set (ATTR_field);
            //TODO: idk lol see the pdf table fig1
            if(node->children.size() == 0)
            {
                break;
            }
            temp_symbol = stack_find_var(left);
            if(temp_symbol)
            {
                if(temp_symbol->attributes[ATTR_struct])
                {
                    field_symbol = table_find_var(temp_symbol->fields,
                                                  right);
                    node->attributes = field_symbol->attributes;
                }
                else
                {
                    fprintf(stderr, "Sorry it's not a struct\n");
                }
            }
            else
            {
                fprintf(stderr, "Sorry doesn't exist, line:5:3\n");
            }
            break;
        case TOK_ARRAY:
            copy_attrs(left, node);
            node->attributes.set(ATTR_array);
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
            new_block();
            // func
            //      name = left
            //      paramlist = middle '('
            //      block = right
            // set each identdecl in
            // paramlist to ATTR_param
            // and ATTR_lval

            left->children[0]->attributes.set(ATTR_function);
            //set return type attrib. Wont this be handled already because postorder?
            //no because this is when TOK_FUNC is encountered, so we have to set the attrib here
            if(left->symbol == TOK_VOID)
                left->children[0]->attributes.set(ATTR_void);
            if(left->symbol == TOK_INT)
                left->children[0]->attributes.set(ATTR_int);
            if(left->symbol == TOK_STRING)
                left->children[0]->attributes.set(ATTR_string);
            if(left->symbol == TOK_TYPEID) {
                left->children[0]->attributes.set(ATTR_struct);
                //TODO we actually have to look up the struct in the table here
                //this is a type

            }


            for (auto child : middle->children) {
                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval); // page 3, 2.2(d)
                //inset into current scope
                define_ident(child->children[0]);
            }
            cout << "\nFunc\n";
            insert_symbol(symbol_stack[0],
                          left->children[0],
                          paramTree_to_symbolVec(middle));

            new_block(); //TODO: should this be gone then?

            set_attributes(right); //this is the block
            // we can recurr over the switch and now handle it in
            // TOK_BLOCK
            return_Vec = new vector<astree*>();
            //not declared in this scope can u copy blockTree_to_returnVec up? yes
            blockTree_to_returnVec (right, return_Vec);
            for(auto return_Node : *return_Vec) //this should work
            {

                if(!compatible_Atrribs(return_Node->attributes,
                                        left->children[0]->attributes))
                {
                    cerr << "incompatible return type";
                    cerr << "\n" << return_Node->attributes<< "\n";
                    cerr << "\n" << left->children[0]->attributes<< "\n";
                }
                /*
                    enum { ATTR_void, ATTR_int, ATTR_null, ATTR_string, ATTR_struct,
                    ATTR_array, ATTR_function, ATTR_variable, ATTR_field, ATTR_typeid,
                    ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr, ATTR_bitset_size };
                */
                //do a for loop for the first 6 elements of the enum. if any dont match throw error
                //with the exception of reference type and null

                //check return_Node->attribs against left->children[0]->attributes

                /*
                    Two types are compatible if they are exactly the same type ; or if one type is
                    any reference type and the other is null. In the type checking grammar, in each
                    rule, types in italics must be substituted consistently by compatible types.
                    Types are compatible only if the array attribute is on for both or off for both.
                */

            }
            exit_block(); //TODO: should this be gone then?
            break;
        case TOK_PROTOTYPE:
            //TODO: check proto
            break;
        case TOK_PARAMLIST:
            break;
        case TOK_DECLID:
            break;
        case TOK_BLOCK:
            new_block();
            for( auto child : node->children)
            {
                //idk, im reading 3.2 right now which deals with this
                set_attributes(child); //is this it? think about the fact that we have to look up the varuables in the table. if it works then yes
            }
            exit_block();
            break; // back
        case TOK_VARDECL:
            // enter into identifier symbol table1
            left->children[0]->attributes.set(ATTR_lval);
            left->children[0]->attributes.set(ATTR_variable);
            copy_attrs(right, left->children[0]);
            copy_attrs(left, node); //Why is this needed?
            if(symbol_stack.size() == 0 || symbol_stack.back() == nullptr)
            {
                //create new symbol table and push onto stack
                //push symbol on newly created symbol table
                temp_symbol = create_symbol(left->children[0]); //TODO: duplicate declaration because can't declare in switch stmt
                symbol_table* temp_table = new symbol_table();
                temp_table->insert (symbol_entry (left->children[0]->lexinfo, temp_symbol)); //TODO: symbol_entry correct?
                symbol_stack.push_back(temp_table);
            }
            else if(table_find_var(symbol_stack.back(), left->children[0])) //checks table of current scope for var
            {
                fprintf(stderr, "Error: Variable <variable> already declared in scope\n" );
            }
            else
            {
                temp_symbol = create_symbol(left->children[0]);
                (symbol_stack.back())->insert (symbol_entry (left->children[0]->lexinfo, temp_symbol));
            }

            //look up in type names table
            //if struct do for loop
            break;
        case TOK_WHILE:
            if(left->attributes[ATTR_void]){
                cerr << "Error not bool\n"; //TODO finish error
            }
            set_attributes(right); //here we recurr on stmt or block
            break;
        case TOK_IF:
            //TODO: check condition?
            //TODO also check if referenct type
            if(left->attributes[ATTR_void]){
                cerr << "Error not bool\n"; //TODO finish error
            }
            if(middle) set_attributes(middle);
            if(right) set_attributes(right);
            break;
        case TOK_ELSE: //TODO: changed from else. SHould be else or ifelse?
            break;
        case TOK_RETURN:
            // I think we should check it in tok_function yyoure right.
            copy_attrs(left, node);
            node->attributes.set(ATTR_struct);
            break;
        case TOK_RETURNVOID:
            node->attributes.set(ATTR_void);
            break;
        case '=':
            //TODO inherit type, typecheck operands
            if(!compatible_Nodes(left,
                                 right))
                {
                    cerr << "error, incompatible types in assignment\n";
                    cerr << *left->lexinfo << " : " << *right->lexinfo;
                }
            break;
        case '+':
            //TODO: should be same as -
            if(!compatible_Nodes(left,
                                 right))
                {
                    cerr << "error, incompatible types in addition\n";
                    cerr << *left->lexinfo << " : " << *right->lexinfo;
                    break;
                }
            copy_attrs(left, node);
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

void postorder (astree* tree)
{
   if (tree == nullptr)
   {
        printf("Tree is null"); //TODO: remove this when done
   }

   for (size_t child = 0; child < tree->children.size(); ++child) {
      postorder(tree->children.at(child));
   }
   printf("\n%d\n", tree->symbol);
   // Call switch func to set attr + typecheck
   set_attributes(tree);
}


void symbol_typecheck(astree* tree)
{
    symbol_stack.push_back(new symbol_table());
    postorder(tree);
}
