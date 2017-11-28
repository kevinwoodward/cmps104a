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
symbol_table* type_table = new symbol_table();
vector<symbol_table*> symbol_stack;
int next_block = 1;

void print_symbol (FILE* outfile, astree* node) {

    cout << "Printing: " << *node->lexinfo << "\n";

    // should be used to print:
    // functions, prototypes, vardecls, structs, and fields of structs

    attr_bitset attributes = node->attributes;

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
            fprintf (outfile, "   %s (%zu.%zu.%zu) field {%s} %s\n",
                (child->children[0]->lexinfo)->c_str(),
                child->children[0]->lloc.linenr, child->children[0]->lloc.filenr, child->children[0]->lloc.offset,
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
                cout << *elem.first << " : " << elem.second->attributes << "\n";
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

symbol* table_find_var(symbol_table* table, const string* lex)
{
    auto symbol_iter = (table->find(lex));
    if (symbol_iter == table->end())  //TODO: is the table-> empty case really needed?
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
            if(currentFirstParam->attributes[j] != currentSecondParam->attributes[j])
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
    ATTR_param, ATTR_lval, ATTR_const, ATTR_vreg, ATTR_vaddr, ATTR_bitset_size };
*/

bool compatible_Primitives(attr_bitset first, attr_bitset second)
{
    for(int i = 0; i < 6; i++)
    {
        if(first[i] && second[i]){
            return true;

        }
    }
    return false;
};

bool compatible_Attribs(attr_bitset first, attr_bitset second)
{
    for(int i = 0; i < 6; i++)
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
        return compatible_Attribs(first->attributes, second->attributes);
    }
    return false;

}

void insert_into_struct_table (symbol_table* table, astree* node)
{
    symbol_table* field_table = new symbol_table();
    for (auto child : node->children) {
        if(child == node->children[0])
            continue;
        child->children[0]->attributes.set(ATTR_field); //delete maybe. Maybe deleted.
        symbol* child_symbol = new symbol;
        child_symbol->attributes = child->children[0]->attributes;
        child_symbol->fields = nullptr;
        child_symbol->filenr = child->children[0]->lloc.filenr;
        child_symbol->linenr = child->children[0]->lloc.linenr;
        child_symbol->offset = child->children[0]->lloc.offset; //OFFSET WOO WOO WOO WOO WOO
        child_symbol->block_nr = child->children[0]->block_nr;
        child_symbol->parameters = nullptr;

        field_table->insert (symbol_entry (child->children[0]->lexinfo, child_symbol));
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
    symbol* temp_symbol2 = nullptr;
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
                //print_symbol(symfile, child->children[0]);  //TODO: this is correct but errors because traversal is wrong
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

            }
            else
            {

                cerr << "undeclared identifier: " << *node->lexinfo << "\n";
                exec::exit_status = 1;
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
                    fprintf(stderr, "Is not a struct\n");
                }
            }
            else
            {
                fprintf(stderr, "Field doesn't exist\n");
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
            node->attributes.set(ATTR_int);
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

            //print_symbol(symfile, left->children[0]);


            for (auto child : middle->children) {
                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval); // page 3, 2.2(d)
                //inset into current scope
                define_ident(child->children[0]);
                //print_symbol(symfile, child->children[0]);
            }


            temp_symbol = table_find_var(symbol_stack[0], left->children[0]);


            if(temp_symbol){
                //TODO: dont use compatible_Attribs here, check w/for()

                if(!compatible_Attribs(temp_symbol->attributes,
                                        left->attributes))
                    {
                        cerr << "incompatible prototype due to ret type";
                        exec::exit_status = 1;
                    }
                if(!compare_params(temp_symbol->parameters,
                                    paramTree_to_symbolVec(middle)))
                    {
                        cerr << "incompatible prototype due to params";
                        exec::exit_status = 1;
                    }


            }

            insert_symbol(symbol_stack[0],
                          left->children[0],
                          paramTree_to_symbolVec(middle));



            //set_attributes(right); //this is the block
            // we can recurr over the switch and now handle it in
            // TOK_BLOCK
            return_Vec = new vector<astree*>();
            //not declared in this scope can u copy blockTree_to_returnVec up? yes
            blockTree_to_returnVec (right, return_Vec);
            for(auto return_Node : *return_Vec) //this should work
            {

                if(!compatible_Attribs(return_Node->attributes,
                                        left->children[0]->attributes))
                {
                    cerr << "incompatible return type";
                    cerr << "\n" << return_Node->attributes<< "\n";
                    cerr << "\n" << left->children[0]->attributes<< "\n";
                    exec::exit_status = 1;
                }

            }
            break;
        case TOK_PROTOTYPE:
            left->children[0]->attributes.set(ATTR_function);

            if(left->symbol == TOK_VOID)
                left->children[0]->attributes.set(ATTR_void);
            if(left->symbol == TOK_INT)
                left->children[0]->attributes.set(ATTR_int);
            if(left->symbol == TOK_STRING)
                left->children[0]->attributes.set(ATTR_string);
            if(left->symbol == TOK_TYPEID)
            {
                //lookup in struct table
                left->children[0]->attributes.set(ATTR_struct);
            }

            //print_symbol(symfile, left->children[0]);


            for (auto child : right->children) {
                child->children[0]->attributes.set(ATTR_variable);
                child->children[0]->attributes.set(ATTR_param);
                child->children[0]->attributes.set(ATTR_lval); // page 3, 2.2(d)
                define_ident(child->children[0]);
                //print_symbol(symfile, child->children[0]);
            }

            insert_symbol(symbol_stack[0],
                          left->children[0],
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
                child->children[0]->attributes.set(ATTR_lval); // page 3, 2.2(d)
                //inset into current scope

                define_ident(child->children[0]);
                print_symbol(symfile, child->children[0]);
            }
            break;
        case TOK_DECLID:
            break;
        case TOK_BLOCK:

            //new_block();
            // for( auto child : node->children)
            // {
            //     //idk, im reading 3.2 right now which deals with this
            //     set_attributes(child);
            // }
            //exit_block();
            break; // back
        case TOK_VARDECL:
            // enter into identifier symbol table1
            left->children[0]->attributes.set(ATTR_lval);
            left->children[0]->attributes.set(ATTR_variable);
            copy_attrs(right, left->children[0]);
            copy_attrs(left, node); //Why is this needed?
            if(!compatible_Nodes(left,
                                 right))
            {
                cerr << "error, incompatible types in assignment\n";
                cerr << *left->lexinfo << " : " << *right->lexinfo;
                cerr << " " << get_attributes(left->attributes) << " : " << get_attributes(right->attributes) << "\n";
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
                    if(table_find_var(symbol_stack.back(), left->children[0]))
                    {
                        cerr << "Duplicate declaraction on variable: " <<
                            *(left->children[0])->lexinfo << "\n";
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
                define_ident(left->children[0]);
                print_symbol(symfile, left->children[0]);
            }
            else
            {
                //then there exists a curr scope, check that one
                if(table_find_var(symbol_stack.back(), left->children[0]))
                {
                    cerr << "Duplicate declaraction on variable: " <<
                        *(left->children[0])->lexinfo << "\n";
                        break;
                }
                else
                {
                    define_ident(left->children[0]);
                    print_symbol(symfile, left->children[0]);
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
        case TOK_ELSE: //TODO: changed from else. SHould be else or ifelse?
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
                    cerr << " " << left->attributes << " : " << right->attributes;
                    exec::exit_status = 1;
                }
            break;
        case '-':
        case '*':
        case '/':
        case '%':
        case '+':
            //TODO: should be same as -
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_vreg);
            temp_symbol = stack_find_var(left);
            temp_symbol2 = stack_find_var(right);
            if(!compatible_Symbols(temp_symbol, temp_symbol2))
                {
                    cerr << "error, incompatible types in addition\n";
                    cerr << *left->lexinfo << " : " << *right->lexinfo;
                    cerr <<  " " <<left->attributes << " : " << right->attributes;
                    exec::exit_status = 1;
                    break;
                }
            copy_attrs(left, node);
            cerr << *node->lexinfo << ": atribs:" << node->attributes << "\n";
            exec::exit_status = 1;
            break;
        case TOK_EQ:
        case TOK_NE:
        case TOK_LE:
        case TOK_GE:
        case TOK_LT:
        case TOK_GT:
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_vreg);
            if(!compatible_Primitives(left->attributes, right->attributes))
            {
                cerr << "incompatible types for comparison";
                exec::exit_status = 1;
            }
            break;
        case TOK_NEG:
        case TOK_POS:
        case '!':
            node->attributes.set(ATTR_int);
            node->attributes.set(ATTR_vreg);
            if(!left->attributes[ATTR_int])
            {
                cerr << "trying to use increment/decrement on non-int";
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
            copy_attrs(left, node);
            node->attributes.set (ATTR_vreg);
            node->attributes.set (ATTR_array);
            break;
        case TOK_CALL:
            //TODO: all of it.
            temp_symbol = stack_find_var(left);
            if(!temp_symbol)
            {
                cerr << "no function of that name\n";
                exec::exit_status = 1;
                break;
            }
            if(node->children.size() - 1 != temp_symbol->parameters->size())
            {
                cerr << "u fuck up somwhere\n";
                cerr << node->children.size() << " ";
                cerr << temp_symbol->parameters->size();
                exec::exit_status = 1;
                break;
            }
            for(size_t i = 0; i < node->children.size() - 1; i++)
            {
                if(!compatible_Primitives(node->children[i+1]->attributes, (temp_symbol->parameters->at(i))->attributes))
                {
                    cerr << "Incompatible types for function call\n";
                    exec::exit_status = 1;
                    break;
                }
            }

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

        for (size_t child = 0; child < tree->children.size(); child++) {
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
        for (size_t child = 0; child < tree->children.size(); child++) {
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
       cout << "GET HERE\n";
       if(      tree->symbol == TOK_FUNCTION
             || tree->symbol == TOK_PROTOTYPE)
       {

           tree->children[0]->children[0]->attributes.set(ATTR_function);

           if(tree->children[0]->symbol == TOK_VOID)
               tree->children[0]->children[0]->attributes.set(ATTR_void);
           if(tree->children[0]->symbol == TOK_INT)
               tree->children[0]->children[0]->attributes.set(ATTR_int);
           if(tree->children[0]->symbol == TOK_STRING)
               tree->children[0]->children[0]->attributes.set(ATTR_string);
           if(tree->children[0]->symbol == TOK_TYPEID)
               tree->children[0]->children[0]->attributes.set(ATTR_struct);



           print_symbol(symfile, tree->children[0]->children[0]);
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
    for (auto child : tree->children)
    {
        traverse(child);
    }
    print_stack();
}
