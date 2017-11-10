%{
// Jose Sepulveda, joasepul@ucsc.edu
// Kevin Woodward, keawoodw@ucsc.edu
// $Id: parser.y,v 1.11 2017-10-11 14:27:30-07 - - $
// Dummy parser for scanner project.

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
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_ROOT

%token TOK_INDEX TOK_NEWSTRING TOK_RETURNVOID TOK_VARDECL
%token TOK_FUNCTION

%right  '='
%left   '+' '-'
%left   '*' '/'
%right  '^'
%right  POS NEG

%start program

%%

start    : program            { yyparse_astree = $1; }
         ;

program  : program structdef  { $$ = $1->adopt ($2); }
         | program function   { $$ = $1->adopt ($2); }
         | program statement  { $$ = $1->adopt ($2); }
         | program error '}'  { $$ = $1; }
         | program error ';'  { $$ = $1; }
         |                    { $$ = parser::root; }
         ;

structdef : TOK_STRUCT TOK_IDENT '{' '}'
                {
                    destroy ($3, $4);
                    $2->symbol = TOK_TYPEID;
                    $$ = $1->adopt($2);
                }
          | TOK_STRUCT TOK_IDENT '{' fielddecl_seq '}'
                {
                    destroy ($3, $5);
                    $2->symbol = TOK_TYPEID;
                    $$ = $1->adopt($2,$4);
                }
		  ;

fielddecl_seq : fielddecl_seq fielddecl ';'
                {
                    destroy($3);
                    $$ = $1->adopt($2);
                }
              |
			  ;

fielddecl   : basetype TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $1->adopt($2);
                }
            | basetype TOK_ARRAY TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $2->adopt($1, $2);
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
            //need to synthesize TOK_FUNCTION
            //or TOK_PROTOTYPE
        }
    | identdecl '(' function_args ')' block
        {

        }
    ;

function_args
    : function_args ',' identdecl
        {
            destroy($2);
            $$ = $1->adopt($3);
        }
    | identdecl
    ;

identdecl   : basetype TOK_IDENT
                {
                    $2->symbol = TOK_DECLID;
                    $$ = $1->adopt($2);
                }
            | basetype TOK_ARRAY TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $2->adopt($1, $2);
                }
            ;
block
    : '{' statement_seq '}'
        {
            destroy($3);
            $$ = $1->adopt($2);
        }
    | ';'
    ;

statement_seq
    : statement_seq statement ';'
        {
            destroy($3);
            $$ = $1->adopt($2);
        }
    |
    ;

statement
    : block
    | vardecl
    | while
    | ifelse
    | return
    | expr ';'
        {
            destroy($2);
            $$ = $1;
        }
    ;

vardecl
    : indentdecl '=' expr ';'
        {
            destroy($4);
            $2->symbol = TOK_VARDECL;
            $$ = $2->adopt($1, $3);
        }
    ;

while
    : TOK_WHILE '(' expr ')' statement
        {
            destroy($2, $4);
            $$ = adopt->($3, $5);
        }
    ;

ifelse
    : TOK_IF '(' expr ')' statement
        {
            destroy($2, $4);
            $$ = $1->adopt($3, $5);
        }
    | TOK_IF '(' expr ')' statement TOK_ELSE statement
        {
            destroy($2);
            destroy($4);
            destory($6);
            $1 = $1->adopt($3);
            $1 = $1->adopt($5);
            $1 = $1->adopt($7);
            $$ = $1;
        }
    ;

return
    : TOK_RETURN ';'
        {
            destroy($2);
            $1->symbol = TOK_RETURNVOID;
            $$ = $1;
        }
    | TOK_RETURN expr ';'
        {
            destroy($3);
            $$ = $1->adopt($2);
        }
    ;

expr    : expr '=' expr         { $$ = $2->adopt ($1, $3); }
        | expr '+' expr         { $$ = $2->adopt ($1, $3); }
        | expr '-' expr         { $$ = $2->adopt ($1, $3); }
        | expr '*' expr         { $$ = $2->adopt ($1, $3); }
        | expr '/' expr         { $$ = $2->adopt ($1, $3); }
        | expr '^' expr         { $$ = $2->adopt ($1, $3); }
        | '+' expr %prec POS    { $$ = $1->adopt_sym ($2, POS); }
        | '-' expr %prec NEG    { $$ = $1->adopt_sym ($2, NEG); }
        | allocator             { $$ = $1; }
        | call                  { $$ = $1; }
        | '(' expr ')'          { destroy ($1, $3); $$ = $2; }
        | variable                { $$ = $1; }
        | constant                { $$ = $1; }
        ;

allocator
    : TOK_NEW TOK_IDENT '(' ')'
        {
            destroy($3, $4);
            $2->symbol = TOK_TYPEID;
            $$ = $1->adopt($2);
        }
    | TOK_NEW TOK_STRING '(' expr ')'
        {
            destroy($2);
            destroy($3);
            destroy($5);
            $1->symbol = TOK_NEWSTRING;
            $$ = $1->adopt($4);
        }
    | TOK_NEW basetype '[' expr ']'
        {
            destroy($3);
            destroy($5);
            $1->symbol = TOK_NEWARRAY;
            $$ = $1->adopt($2, $4);

        }
    ;

call
    : TOK_IDENT '(' ')'
        {
            destroy($3);
            $2->symbol = TOK_CALL;
            $$ = $1->adopt($2);
        }
    | TOK_IDENT '(' function_params ')'
        {
            destroy($4);
            $2 = $2->adopt($3);
            $$ = $1->adopt($2);
        }
    ;

function_params
    : function_params ',' expr
        {
            destroy($2);
            $$ = $1->adopt($3);
        }
    | expr
    ;

variable
    : TOK_IDENT
    | expr '[' expr ']'
    | expr '.' TOK_FIELD
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
