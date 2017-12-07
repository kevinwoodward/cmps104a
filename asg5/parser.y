%{
// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu

#include <cassert>

#include "lyutils.h"
#include "astree.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token TOK_VOID TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_DECLID TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID
%token TOK_ORD TOK_CHR TOK_ROOT TOK_PARAN TOK_UNI

%token TOK_INDEX TOK_FIELD TOK_NEWSTRING TOK_RETURNVOID TOK_VARDECL
%token TOK_FUNCTION TOK_PARAMLIST TOK_PROTOTYPE

%precedence TOK_IF
%precedence TOK_ELSE
%right  '='
%left   TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left   '+' '-'
%left   '*' '/' '%'
%precedence  TOK_UNI
%precedence   '[' '.'

%%

start
    : program            { parser::root = $1; }
    ;

program
    : program structdef { $$ = $1->adopt ($2); }
    | program function  { $$ = $1->adopt ($2); }
    | program statement { $$ = $1->adopt ($2); }
    | program error '}' { $$ = $1; }
    | program error ';' { $$ = $1; }
    | %empty            { $$ = astree::synthesize_root(parser::root); }

    ;

structdef
    : TOK_STRUCT TOK_IDENT '{' '}'
        {
            destroy ($3, $4);
            $$ = $1->adopt_with_sym(TOK_TYPEID, $2);
        }
    | fielddecl_seq '}'
        {
            destroy ($2);
            $$ = $1;
        }
    ;

fielddecl_seq
    : fielddecl_seq fielddecl
        {
            $$ = $1->adopt($2);
        }
    | TOK_STRUCT TOK_IDENT '{' fielddecl
        {
            destroy ($3);
            $$ = $1->adopt_with_sym(TOK_TYPEID, $2, $4);
        }
    ;

fielddecl
    : basetype TOK_IDENT ';'
        {
            destroy ($3);
            $$ = $1->adopt_with_sym(TOK_FIELD, $2);
        }
    | basetype TOK_ARRAY TOK_IDENT ';'
        {
            destroy($4);
            $$ = $2->adopt_sym(TOK_FIELD ,$1, $3);
        }
    ;

basetype
    : TOK_VOID
    | TOK_INT
    | TOK_STRING
    | TOK_IDENT
        {
            $1->symbol = TOK_TYPEID;
            $$ = $1;
        }
    ;

function
    : identdecl '(' ')' block
        {
            destroy($3);
            $$ = astree::synthesize_function($1, $2, $4);
        }
    | identdecl func_params ')' block
        {
            destroy($3);
            $$ = astree::synthesize_function($1, $2, $4);
        }
    ;

func_params
    : func_params ',' identdecl
        {
            destroy($2);
            $$ = $1->adopt($3);
        }
    | '(' identdecl
        {
            $$ = $1->adopt_sym(TOK_PARAMLIST, $2);
        }
    ;

identdecl
    : basetype TOK_IDENT
        {
            $$ = $1->adopt_with_sym(TOK_DECLID, $2);
        }
    | basetype TOK_ARRAY TOK_IDENT
        {
            $3->symbol = TOK_DECLID;
            $$ = $2->adopt($1, $3);
        }
    ;
block
    :  statement_seq '}'
        {
            destroy($2);
            $$ = $1;
        }
    | ';'
    ;



statement_seq
    : statement_seq statement
        {
            $$ = $1->adopt($2);
        }
    | '{' statement
        {
            $$ = $1->adopt_sym(TOK_BLOCK, $2);
        }
    ;

statement
    : block
    | vardecl ';'
    | while
    | ifelse
    | return ';'
    | expr ';'
    ;

vardecl
    : identdecl '=' expr
        {
            $$ = $2->adopt_sym(TOK_VARDECL, $1, $3);
        }
    ;

while
    : TOK_WHILE '(' expr ')' statement
        {
            destroy($2, $4);
            $$ = $1->adopt($3, $5);
        }
    ;

ifelse
    : TOK_IF '(' expr ')' statement %prec TOK_IF
        {
            destroy($2, $4);
            $$ = $1->adopt_ifelse($3, $5);
        }
    | TOK_IF '(' expr ')' statement TOK_ELSE statement %prec TOK_ELSE
        {
            destroy($2, $4, $6);
            $$ = $1->adopt_ifelse($3, $5, $7);
        }
    ;

return
    : TOK_RETURN
        {
            $1->symbol = TOK_RETURNVOID;
            $$ = $1;
        }
    | TOK_RETURN expr
        {
            $$ = $1->adopt($2);
        }
    ;

expr
    : expr '=' expr             { $$ = $2->adopt ($1, $3); }
    | expr '+' expr             { $$ = $2->adopt ($1, $3); }
    | expr '-' expr             { $$ = $2->adopt ($1, $3); }
    | expr '*' expr             { $$ = $2->adopt ($1, $3); }
    | expr '/' expr             { $$ = $2->adopt ($1, $3); }
    | expr '%' expr             { $$ = $2->adopt ($1, $3); }
    | expr TOK_EQ expr          { $$ = $2->adopt ($1, $3); }
    | expr TOK_NE expr          { $$ = $2->adopt ($1, $3); }
    | expr TOK_LE expr          { $$ = $2->adopt ($1, $3); }
    | expr TOK_GE expr          { $$ = $2->adopt ($1, $3); }
    | expr TOK_LT expr          { $$ = $2->adopt ($1, $3); }
    | expr TOK_GT expr          { $$ = $2->adopt ($1, $3); }
    | '+' expr %prec TOK_UNI    { $$ = $1->adopt_sym (TOK_POS, $2); }
    | '-' expr %prec TOK_UNI    { $$ = $1->adopt_sym (TOK_NEG, $2); }
    | '!' expr %prec TOK_UNI    { $$ = $1->adopt($2); }
    | allocator                 { $$ = $1; }
    | call                      { $$ = $1; }
    | '(' expr ')'              { destroy ($1, $3); $$ = $2; }
    | variable                  { $$ = $1; }
    | constant                  { $$ = $1; }
    ;

allocator
    : TOK_NEW TOK_IDENT '(' ')'
        {
            destroy($3, $4);
            $$ = $1->adopt_with_sym(TOK_TYPEID, $2);
        }
    | TOK_NEW TOK_STRING '(' expr ')'
        {
            destroy($2, $3, $5);
            $$ = $1->adopt_sym(TOK_NEWSTRING, $4);
        }
    | TOK_NEW basetype '[' expr ']'
        {
            destroy($3, $5);
            $$ = $1->adopt_sym(TOK_NEWARRAY, $2, $4);

        }
    ;

call
    : TOK_IDENT '(' ')'
        {
            destroy($3);
            $$ = $2->adopt_sym(TOK_CALL, $1);
        }
    | call_args ')'
        {
            destroy($2);
            $$ = $1;
        }
    ;

call_args
    : call_args ',' expr
        {
            destroy($2);
            $$ = $1->adopt($3);
        }
    | TOK_IDENT '(' expr
        {
            $$ = $2->adopt_sym(TOK_CALL, $1, $3);
        }
    ;

variable
    : TOK_IDENT
    | expr '[' expr ']'
        {
            destroy($4);
            $$ = $2->adopt_sym(TOK_INDEX, $1, $3);
        }
    | expr '.' TOK_IDENT
        {
            $3->symbol = TOK_FIELD;
            $$ = $2->adopt($1, $3);
        }
    ;

constant
    : TOK_INTCON
    | TOK_CHARCON
    | TOK_STRINGCON
    | TOK_NULL
    ;

%%


const char *parser::get_tname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

/*
static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != nullptr);
   return result;
}
*/
