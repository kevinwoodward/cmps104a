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
FILE* symfile;

symbol_table* struct_table = new symbol_table();
vector<symbol_table*> symbol_stack;
int next_block = 1;
bool last_printed_nl = false;

void print_symbol (FILE* outfile, astree* node) {
    attr_bitset attributes = node->attributes;
    last_printed_nl = false;

    for(size_t i = 0; i < symbol_stack.size(); i++)
    {
        fprintf (outfile, "   ");
    }

    if (node->attributes[ATTR_field]) {
         fprintf (outfile, "%s (%zu.%zu.%zu) field {%s} ",
             (node->lexinfo)->c_str(),
             node->lloc.linenr, node->lloc.filenr, node->lloc.offset,
             "current struct");
    } else {
        fprintf (outfile, "%s (%zu.%zu.%zu) {%d} ",
            (node->lexinfo)->c_str(),
            node->lloc.linenr, node->lloc.filenr, node->lloc.offset,
            node->block_nr);
    }

    if (node->symbol == TOK_STRUCT) {
        fprintf (outfile, "struct \"%s\" \n",
            (node->children[0]->lexinfo)->c_str());
        for (auto child : node->children) {
            if(child == node->children[0])
                continue;
            fprintf (outfile, "   %s (%zu.%zu.%zu) field {%s} %s\n\n",
                (child->children[0]->lexinfo)->c_str(),
                child->children[0]->lloc.linenr,
                child->children[0]->lloc.filenr,
                child->children[0]->lloc.offset,
                (node->children[0]->lexinfo)->c_str(),
                get_attributes (child->children[0]->attributes));

        }

    }
    fprintf (outfile, "%s\n", get_attributes (attributes));
}

void print_stack()
{
    cout << "Printing stack, size is: " << symbol_stack.size() << "\n";
    int i = 0;
    for(auto symtab : symbol_stack)
    {

        if (symtab != nullptr)
        {
            cout << "stack: " << i << ": \n";
            for (auto elem : *symtab)
            {
                cout << *elem.first <<
                " : " << elem.second->attributes << "\n";
            }
            i = i + 1;
        }
        else
        {
            cout << "nullblock\n";
        }


    }
}

void new_block()
{
    next_block++;
    symbol_stack.push_back(nullptr);
}

void exit_block()
{
    symbol_stack.pop_back();
    if(!last_printed_nl)
    {
        fprintf(symfile, "\n");
        last_printed_nl = true;
    }
    last_printed_nl = true;
}

symbol* table_find_var(symbol_table* table, astree* node)
{
    auto symbol_iter = (table->find(node->lexinfo));
    //TODO: is the table-> empty case really needed?
    if (symbol_iter == table->end())
    {
        return nullptr;
    }
    return (table->find (node->lexinfo))->second;
}

symbol* table_find_var(symbol_table* table, const string* lex)
{
    auto symbol_iter = (table->find(lex));
    //TODO: is the table-> empty case really needed?
    if (symbol_iter == table->end())
    {
        return nullptr;
    }
    return (table->find (lex))->second;
}



symbol* stack_find_var(astree* node)
{

    symbol_table* current_table = nullptr;
    for(int i = symbol_stack.size(); i > 0; i--)
    {
        current_table = symbol_stack[i-1];
        if(current_table != nullptr)
        {
            if(!(current_table->empty()))
            {
                symbol* result = table_find_var(current_table, node);
                if(result != nullptr)
                {
                    return result;
                }
            }
        }
    }
    return nullptr;
}

void blockTree_to_returnVec(astree* tree, vector<astree*>* &returnVec)
{
   if (tree == nullptr)
   {
        return;
   }

   for (size_t child = 0; child < tree->children.size(); ++child)
   {
      blockTree_to_returnVec(tree->children.at(child), returnVec);
   }
   //also chekc if return or returnvoid.
   //but they wont have attributes yet

   //this works
   if(tree->symbol == TOK_RETURN || tree->symbol == TOK_RETURNVOID)
   {
       returnVec->push_back(tree);
   }
   return;

}

symbol* create_symbol (astree* node,
                       vector<symbol*>* parameters = nullptr)
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
    node->block_nr = next_block - 1;
    symbol* symbol = create_symbol (node, paramVec);
    if ((table != nullptr) && (node != nullptr)) {
        table->insert (symbol_entry (node->lexinfo, symbol));
    }
}
void insert_struct (symbol_table* table,
                    astree* node,
                    symbol_table* field_table) {
    node->block_nr = next_block - 1;
    symbol* symbol = create_symbol (node);
    symbol->fields = field_table;

    if ((table != nullptr) && (node != nullptr)) {
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

bool compare_params(vector<symbol*>* first, vector<symbol*>* second)
{
    symbol* currentFirstParam;
    symbol* currentSecondParam;
    if(first->size() != second->size())
    {
        return false;
    }
    for(size_t i = 0; i < first->size(); i++)
    {
        currentFirstParam = first->at(i);
        currentSecondParam = second->at(i);
        for(size_t j = 0; j < 7; j++)
        {
            if(currentFirstParam->attributes[j]
                != currentSecondParam->attributes[j])
            {
                return false;
            }
        }
    }
    return true;
}

void define_ident (astree* node) {
    if (symbol_stack.back() == nullptr) {
        symbol_stack.pop_back();
        symbol_stack.push_back( new symbol_table ());
    }
    symbol_table* table = symbol_stack.back();
    insert_symbol (table, node);
}

void define_struct (astree* node) {
    if (symbol_stack.back() == nullptr) {
        symbol_stack.pop_back();
        symbol_stack.push_back( new symbol_table ());
    }
    symbol* struct_symbol = nullptr;
    symbol_table* table = symbol_stack.back();
    //look up field table in struct_table with node->lexinfo
    symbol_table* field_table = nullptr;
    struct_symbol = table_find_var(struct_table, node);
    if(struct_symbol)
    {
        field_table = struct_symbol->fields;
        node->children[0]->attributes.reset(ATTR_typeid);
        node->children[0]->attributes.set(ATTR_struct);
        insert_struct (table, node->children[0], field_table);
    }
    else {
       cerr << "No definition for: " << *node->lexinfo << "\n";
       exec::exit_status = 1;
       return;
    }


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
    ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr,
    ATTR_bitset_size };
*/

bool compatible_Primitives(attr_bitset first, attr_bitset second)
{
    for(int i = 0; i < 5; i++)
    {
        if(first[i] && second[i]){
            return true;

        }
    }
    return false;
};

bool compatible_Attribs(attr_bitset first, attr_bitset second)
{
    for(int i = 0; i < 5; i++)
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
    reference = first[ATTR_string]
                     || first[ATTR_array]
                     || first[ATTR_struct];
    if(second[ATTR_null] && reference)
    {
        return true;
    }

    if(first[ATTR_typeid] && second[ATTR_typeid])
    {
        return true;
    }
    return false;


}

bool compatible_structs(const string* first, const string* second)
{

    if(*first == *second)
    {
        symbol* struct_symbol = nullptr;
        //look up field table in struct_table with node->lexinfo
        struct_symbol = table_find_var(struct_table, first);
        if(struct_symbol)
        {
            return true;
        }
    }
    return false;
}

bool compatible_Nodes(astree* first, astree* second)
{
   return compatible_Attribs(first->attributes, second->attributes);

}

bool compatible_Symbols(symbol* first, symbol* second)
{

    if(first != nullptr && second != nullptr)
    {
        return compatible_Attribs(first->attributes,
                                  second->attributes);
    }
    return false;

}

void insert_into_struct_table (symbol_table* table, astree* node)
{
    symbol_table* field_table = new symbol_table();
    for (auto child : node->children) {
        if(child == node->children[0])
            continue;
        child->children[0]->attributes.set(ATTR_field);
        symbol* child_symbol = new symbol;
        child_symbol->attributes = child->children[0]->attributes;
        child_symbol->fields = nullptr;
        child_symbol->filenr = child->children[0]->lloc.filenr;
        child_symbol->linenr = child->children[0]->lloc.linenr;
        child_symbol->offset = child->children[0]->lloc.offset;
        child_symbol->block_nr = child->children[0]->block_nr;
        child_symbol->parameters = nullptr;

        field_table->insert (symbol_entry (child->children[0]->lexinfo,
                                           child_symbol));
    }
    symbol* node_symbol = new symbol;
    node_symbol->attributes = node->attributes;
    node_symbol->fields = field_table;
    node_symbol->filenr = node->lloc.filenr;
    node_symbol->linenr = node->lloc.linenr;
    node_symbol->offset = node->lloc.offset; //OFFSET WOO WOO WOO WOO
    node_symbol->block_nr = node->block_nr;
    node_symbol->parameters = nullptr;

    table->insert (symbol_entry (node->children[0]->lexinfo,
                                 node_symbol));
}


/*
    QTION TOK_PROTOTYPE
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
    astree* temp_ast_node = nullptr;
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
    //symbol* temp_symbol2 = nullptr;
    symbol* field_symbol = nullptr;
    vector<astree*>* return_Vec = nullptr;

    switch (node->symbol) {
        case TOK_ROOT:
            break;
        case TOK_STRUCT:
            //TODO: insert into type name table
            // insert into struct table with all other defs

            left->attributes.set(ATTR_struct);
            left->attributes.set(ATTR_typeid);
            print_symbol(symfile, node);
            for (auto child : node->children) {
                if(child == node->children[0])
                    continue;
                child->attributes.set(ATTR_field);

            }
            insert_into_struct_table(struct_table, node);

            break;
        case TOK_IDENT:
            // Get attributes from symtab stack
            //TODO: if temp_symbol is null then check types
                //if null after that, throw error: "not found"

            temp_symbol = stack_find_var(node);

            if(temp_symbol != nullptr)
            {

                node->attributes = temp_symbol->attributes;

            }
            else
            {

                cerr << "undeclared identifier: "
                     << *node->lexinfo << "\n";
                 print_stack();
                exec::exit_status = 1;
            }
            break;
        case TOK_TYPEID:
            node->attributes.set(ATTR_typeid);
            break;
        case TOK_FIELD:
            node->attributes.set(ATTR_field);
            break;
        case '.':
            node->attributes.set (ATTR_field);
            //TODO: idk lol see the pdf table fig1

            // if(node->children.size() == 0)
            // {
            //     break;
            // }

            temp_symbol = stack_find_var(left);
            if(temp_symbol)
            {
                if(temp_symbol->attributes[ATTR_struct])
                {
                    field_symbol = table_find_var(temp_symbol->fields,
                                                  right);
                    node->attributes = field_symbol->attributes;

                    node->attributes.set (ATTR_vaddr);
                    node->attributes.set (ATTR_lval);

                }
                else
                {
                    fprintf(stderr, "Is not a struct\n");
                }
            }
            else
            {
                fprintf(stderr, "Field doesn't exist\n");
            }
            break;
        case TOK_ARRAY:
            if(left->symbol == TOK_INT)
                node->attributes.set(ATTR_int);
            if(left->symbol == TOK_STRING)
                node->attributes.set(ATTR_string);
            if(left->symbol == TOK_TYPEID)
                node->attributes.set(ATTR_struct);
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
            node->attributes.set(ATTR_int);
            //TODO: inherit type
            break;
        case TOK_STRING:
            if(left == nullptr)
            {
                break;
            }
            left->attributes.set(ATTR_string);
            node->attributes.set(ATTR_string);
            //TODO: inherit type
            break;

        case TOK_FUNCTION:

            // func
            //      name = left
            //      paramlist = middle '('
            //      block = right
            // set each identdecl in
            // paramlist to ATTR_param
            // and ATTR_lval

            if(left->symbol == TOK_ARRAY)
            {
                temp_ast_node = left->children[1];
                temp_ast_node->attributes.set(ATTR_array);
            }
            else
            {
                temp_ast_node = left->children[0];
            }

            for (auto child : middle->children) {
                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval);
                //inset into current scope
                define_ident(child->children[0]);
                //print_symbol(symfile, child->children[0]);
            }


            temp_symbol = table_find_var(symbol_stack[0],
                                         temp_ast_node);


            if(temp_symbol){
                //TODO: dont use compatible_Attribs here, check w/for()

                if(!compatible_Attribs(temp_symbol->attributes,
                                        left->attributes))
                    {
                        cerr << "incompatible prototype "
                                "due to ret type";
                        exec::exit_status = 1;
                    }
                if(!compare_params(temp_symbol->parameters,
                                    paramTree_to_symbolVec(middle)))
                    {
                        cerr << "incompatible prototype due to params";
                        exec::exit_status = 1;
                    }


            }

            //insert_symbol(symbol_stack[0],
            //              temp_ast_node,
            //              paramTree_to_symbolVec(middle)); //TODO delete or uncomment



            //set_attributes(right); //this is the block
            // we can recurr over the switch and now handle it in
            // TOK_BLOCK
            return_Vec = new vector<astree*>();
            blockTree_to_returnVec (right, return_Vec);
            for(auto return_Node : *return_Vec) //this should work
            {

                if(!compatible_Attribs(return_Node->attributes,
                                        temp_ast_node->attributes))
                {
                    cerr << "incompatible return type";
                    exec::exit_status = 1;
                }

            }
            break;
        case TOK_PROTOTYPE:

            if(left->symbol == TOK_ARRAY)
            {
                temp_ast_node = left->children[1];
                temp_ast_node->attributes.set(ATTR_array);
            }
            else
            {
                temp_ast_node = left->children[0];
            }

            for (auto child : right->children) {
                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval);
                define_ident(child->children[0]);
                //print_symbol(symfile, child->children[0]);
            }

            insert_symbol(symbol_stack[0],
                          temp_ast_node,
                          paramTree_to_symbolVec(right));

            break;
        case TOK_PARAMLIST:
            for (auto child : node->children) {
                if(child->symbol == TOK_VOID)
                    child->children[0]->attributes.set(ATTR_void);
                if(child->symbol == TOK_INT)
                    child->children[0]->attributes.set(ATTR_int);
                if(child->symbol == TOK_STRING)
                    child->children[0]->attributes.set(ATTR_string);
                if(child->symbol == TOK_TYPEID)
                    child->children[0]->attributes.set(ATTR_struct);

                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval);
                //inset into current scope

                define_ident(child->children[0]);
                print_symbol(symfile, child->children[0]);
            }
            break;
        case TOK_DECLID:
            break;
        case TOK_BLOCK:


            break;
        case TOK_VARDECL:
            //TODO: @JOSE check if left is tok_array and check if right
                    //has attr_array
            if(left->symbol == TOK_ARRAY)
            {
                temp_ast_node = left->children[1];

            }
            else
            {
                temp_ast_node = left->children[0];
            }
            temp_ast_node->attributes.set(ATTR_lval);
            temp_ast_node->attributes.set(ATTR_variable);
            copy_attrs(right, temp_ast_node);
            copy_attrs(left, node); //Why is this needed?
            if(!compatible_Nodes(left,
                                 right))
            {
                cerr << "error, incompatible types in assignment\n";
                cerr << *left->lexinfo << " : " << *right->lexinfo;
                cerr << " " << get_attributes(left->attributes)
                << " : " << get_attributes(right->attributes) << "\n";
                exec::exit_status = 1;
            }

            if(left->symbol == TOK_TYPEID)
            {
                //check to see if foo = foo() in foo f = new foo();
                if(*(left->lexinfo) != *(right->children[0]->lexinfo))
                {
                    cerr << "Incompatible struct declaration: "
                        << *(left->lexinfo) << " and "
                        << *(right->children[0]->lexinfo) << "\n";
                    exec::exit_status = 1;
                    break;
                }
                //look up in struct table
                if(symbol_stack.back() == nullptr)
                {
                    //then new scope
                    define_struct(left);
                    print_symbol(symfile, left->children[0]);
                }
                else
                {
                    //then there exists a curr scope, check that one
                    if(table_find_var(symbol_stack.back(),
                                      left->children[0]))
                    {
                        cerr << "Duplicate declaraction on variable: "
                             << *(left->children[0])->lexinfo << "\n";
                        exec::exit_status = 1;
                        break;
                    }
                    else
                    {
                        define_struct(left);
                        print_symbol(symfile, left->children[0]);
                    }

                }
                break;
            }
            //TODO: simplify logic
            if(symbol_stack.back() == nullptr)
            {
                //then new scope
                define_ident(temp_ast_node);
                print_symbol(symfile, temp_ast_node);
            }
            else
            {
                //then there exists a curr scope, check that one
                if(table_find_var(symbol_stack.back(),
                                  temp_ast_node))
                {
                    cerr << "Duplicate declaraction on variable: "
                        << *temp_ast_node->lexinfo << "\n";
                    exec::exit_status = 1;
                    break;
                }
                else
                {
                    define_ident(temp_ast_node);
                    print_symbol(symfile, temp_ast_node);
                }

            }


            break;
        case TOK_WHILE:
            if(left->attributes[ATTR_void]){
                cerr << "Error not bool\n";
                exec::exit_status = 1;
            }
            set_attributes(right); //here we recurr on stmt or block
            break;
        case TOK_IF:
            //TODO: check condition?
            //TODO also check if referenct type
            if(left->attributes[ATTR_void]){
                cerr << "Error not bool\n"; //TODO finish error
                exec::exit_status = 1;
            }
            if(middle) set_attributes(middle);
            if(right) set_attributes(right);
            break;
        case TOK_ELSE:
            break;
        case TOK_RETURN:
            // I think we should check it in Qtion yyoure right.
            copy_attrs(left, node);
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
                    cerr << " " << left->attributes
                         << " : " << right->attributes << "\n";
                    exec::exit_status = 1;
                }
            if(!left->attributes[ATTR_lval])
            {
                cerr << "ERROR: " << *left->lexinfo
                    << " cannot be assigned to.";
                exec::exit_status = 1;
                break;
            }
            node->attributes.set(ATTR_vreg);
            break;
        case '-':
        case '*':
        case '/':
        case '%':
        case '+':
            //TODO: should be same as -
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_vreg);
            if(!(left->attributes[ATTR_int]
                && right->attributes[ATTR_int]))
                {
                    cerr << "error, incompatible types in operation\n";
                    cerr << *left->lexinfo << " : " << *right->lexinfo;
                    cerr <<  " " <<left->attributes
                         << " : " << right->attributes << "\n";
                    exec::exit_status = 1;
                    break;
                }
            copy_attrs(left, node);
            break;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LE:
        case TOK_GE:
        case TOK_LT:
        case TOK_GT:
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_vreg);
            set_attributes(left);
            set_attributes(right);
            if(!compatible_Attribs(left->attributes,
                                      right->attributes))
            {
                cerr << "incompatible types for comparison\n";
                cerr << "L, R : " << *left->lexinfo << ", " << *right->lexinfo << "\n";
                exec::exit_status = 1;
            }
            break;
        case TOK_NEG:
        case TOK_POS:
        case '!':
            node->attributes.set(ATTR_vreg);
            set_attributes(left);
            if(!left->attributes[ATTR_int])
            {
                cerr << "trying to use increment/decrement on non-int\n";
                exec::exit_status = 1;
            }
            else
            {
                copy_attrs(left, node);
            }
            break;
        case TOK_NEW:
            copy_attrs(left, node);
            break;
        case TOK_NEWSTRING:
            node->attributes.set (ATTR_vreg);
            node->attributes.set (ATTR_string);
            break;
        case TOK_NEWARRAY:
            if(left->symbol == TOK_INT)
                node->attributes.set(ATTR_int);
            if(left->symbol == TOK_STRING)
                node->attributes.set(ATTR_string);
            if(left->symbol == TOK_TYPEID)
                node->attributes.set(ATTR_struct);

            node->attributes.set (ATTR_vreg);
            node->attributes.set (ATTR_array);
            break;
        case TOK_CALL:
            //TODO: all of it.
            for(auto child : node->children)
            {
                set_attributes(child);
            }
            temp_symbol = stack_find_var(left);
            copy_attrs(left, node);
            node->attributes.reset(ATTR_function);
            if(!temp_symbol)
            {
                cerr << "no function of that name\n";
                exec::exit_status = 1;
                break;
            }
            if(node->children.size()-1
               != temp_symbol->parameters->size())
            {
                cerr << "u fuck up somwhere\n";
                cerr << node->children.size() << " ";
                cerr << temp_symbol->parameters->size();
                exec::exit_status = 1;
                break;
            }
            for(size_t i = 0; i < node->children.size() - 1; i++)
            {
                if(!compatible_Primitives((node->children[i+1])->
                                              attributes,
                                          (temp_symbol->parameters->
                                              at(i))->attributes))
                {
                    cerr << "Incompatible types for function call\n";
                    cerr << *left->lexinfo << "\n";
                    cerr <<
                        get_attributes(node->children[i+1]->attributes)
                        << " : "
                        << get_attributes(
                            temp_symbol->parameters->at(i)->attributes)
                            << "\n";
                    exec::exit_status = 1;
                    break;
                }

            }

            break;
        case TOK_INDEX:
            //TODO: fill out rest
            copy_attrs(left, node);
            node->attributes.reset(ATTR_array);
            if(left->attributes[ATTR_string]
                && !left->attributes[ATTR_array])
            {
                node->attributes.reset(ATTR_string);
                node->attributes.set(ATTR_int);
            }
            else if (left->attributes[ATTR_int]
                && left->attributes[ATTR_array])
            {
                node->attributes.set(ATTR_int);
            }

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

/*
Here we need code to do a traversal
through the astree and set the different attributes
this is called after the parser.y returns the tree
to main, then we call this with the tree and outfile
*/

void postorder (astree* tree)
{
    if(      tree->symbol == TOK_BLOCK
             || tree->symbol == TOK_PARAMLIST)
    {
        new_block();
    }
        //tree->block_nr = next_block;

        for (size_t child = 0; child < tree->children.size(); child++){
           postorder(tree->children.at(child));
        }
        set_attributes(tree);

    if(tree->symbol == TOK_BLOCK)
    {
        exit_block();
    }
}

void preorder (astree* tree)
{
    if(      tree->symbol == TOK_BLOCK)
    {
        new_block();
    }
        //tree->block_nr = next_block;
        set_attributes(tree);
        for (size_t child = 0; child < tree->children.size(); child++){
           preorder(tree->children.at(child));
        }

    if(      tree->symbol == TOK_BLOCK)
    {
        exit_block();
    }
}

void traverse (astree* tree)
{
   if (tree == nullptr)
   {
        printf("Tree is null"); //TODO: remove this when done
   }
   if(      tree->symbol == TOK_BLOCK)
   {
       new_block();
       //tree->block_nr = next_block;
       preorder(tree);
       exit_block();
   }
   else
   {
       if(      tree->symbol == TOK_FUNCTION
             || tree->symbol == TOK_PROTOTYPE)
       {
           astree* funcNode = nullptr;
           astree* returnNode = nullptr;
           if(tree->children[0]->symbol == TOK_ARRAY)
           {
               funcNode = tree->children[0]->children[1];
               returnNode = tree->children[0]->children[0];
               funcNode->attributes.set(ATTR_array);
           }
           else
           {
               funcNode = tree->children[0]->children[0];
               returnNode = tree->children[0];
           }
           funcNode->attributes.set(ATTR_function);

           if(returnNode->symbol == TOK_VOID)
               funcNode->attributes.set(ATTR_void);
           if(returnNode->symbol == TOK_INT)
               funcNode->attributes.set(ATTR_int);
           if(returnNode->symbol == TOK_STRING)
               funcNode->attributes.set(ATTR_string);
           if(returnNode->symbol == TOK_TYPEID)
               funcNode->attributes.set(ATTR_struct);



           print_symbol(symfile, funcNode);
       }
       postorder(tree);
       if(      tree->symbol == TOK_FUNCTION
             || tree->symbol == TOK_PROTOTYPE)
       {
           exit_block();
       }
   }

}


void symbol_typecheck(astree* tree, FILE* outfile)
{
    symfile = outfile;
    symbol_stack.push_back(new symbol_table()); //TODO: maybe change
    if(tree == nullptr)
    {
        cerr << "Root tree is null\n";
        exec::exit_status = 1;
        return;
    }
    for (auto child : tree->children)
    {
        traverse(child);
    }
    //print_stack();
}
