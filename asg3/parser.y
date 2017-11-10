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
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID
%token TOK_ORD TOK_CHR TOK_ROOT TOK_PARAN TOK_UNI

%token TOK_INDEX TOK_FIELD TOK_NEWSTRING TOK_RETURNVOID TOK_VARDECL
%token TOK_FUNCTION TOK_PARAMLIST TOK_PROTOTYPE

%precedence TOK_IDENT
%precedence TOK_IF
%precedence TOK_ELSE
%right  '='
%left   TOK_EQ TOK_NE '<' TOK_LE '>' TOK_GE
%left   '+' '-'
%left   '*' '/' '%'
%precedence  TOK_UNI
%precedence   '[' '.' '('





%%

start    : program            { parser::root = $1; }
         ;

program  : program structdef  { $$ = $1->adopt ($2); }
         | program function   { $$ = $1->adopt ($2); }
         | program statement  { $$ = $1->adopt ($2); }
         | program error '}'  { $$ = $1; }
         | program error ';'  { $$ = $1; }
         | %empty
            {

             $$ = astree::synthesize_root(parser::root);
            }
         ;

structdef : TOK_STRUCT TOK_IDENT '{' '}'
                {
                    destroy ($3, $4);
                    $2->symbol = TOK_TYPEID;
                    $$ = $1->adopt($2);
                }
          | fielddecl_seq '}'
                {
                    destroy ($2);

                    $$ = $1;
                }
		  ;

fielddecl_seq : fielddecl_seq fielddecl ';'
                {
                    destroy($3);
                    $$ = $1->adopt($2);
                }
              | TOK_STRUCT TOK_IDENT '{' fielddecl
                  {
                      destroy ($3);
                      $2->symbol = TOK_TYPEID;
                      $$ = $1->adopt($2, $4);
                  }
			  ;

fielddecl   : basetype TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $1->adopt($2);
                }
            | basetype TOK_ARRAY TOK_IDENT
                {
                    $2->symbol = TOK_FIELD;
                    $$ = $2->adopt($1, $3);
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
            $2->symbol = TOK_PARAMLIST;
            if($4->symbol == ';'){
                destroy($4);
                $$ = $1->synthesize_prototype(TOK_PROTOTYPE, $1, $2);
            }
            else
                $$ = $1->astree::synthesize_function(TOK_FUNCTION, $1, $2, $4);

        }
    | identdecl '(' func_params ')' block
        {
            destroy($4);
            $2->symbol = TOK_PARAMLIST;
            $2 = $2->adopt($3);
            if($5->symbol == ';'){
                destroy($5);
                $$ = $1->synthesize_prototype(TOK_PROTOTYPE, $1, $2);
            }
            else
                $$ = $1->synthesize_function(TOK_FUNCTION, $1, $2, $5);

        }
    ;

func_params
    : func_params ',' identdecl
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
            $1->symbol = TOK_BLOCK;
            $$ = $1->adopt($2);
        }
    ;

statement
    : block
    | vardecl
    | while
    | ifelse
    | return
    | expr ';'
    ;

vardecl
    : identdecl '=' expr ';'
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
            $$ = $1->adopt($3, $5);
        }
    ;

ifelse
    : TOK_IF '(' expr ')' statement %prec TOK_IF
        {
            destroy($2, $4);
            $$ = $1->adopt($3, $5);
        }
    | TOK_IF '(' expr ')' statement TOK_ELSE statement %prec TOK_ELSE
        {
            destroy($2);
            destroy($4);
            destroy($6);
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
        | expr '%' expr         { $$ = $2->adopt ($1, $3); }
        | expr TOK_EQ expr         { $$ = $2->adopt ($1, $3); }
        | expr TOK_NE expr         { $$ = $2->adopt ($1, $3); }
        | expr TOK_LE expr         { $$ = $2->adopt ($1, $3); }
        | expr TOK_GE expr         { $$ = $2->adopt ($1, $3); }
        | expr '<' expr         { $$ = $2->adopt ($1, $3); }
        | expr '>' expr         { $$ = $2->adopt ($1, $3); }
        | '+' expr %prec TOK_UNI  { $$ = $1->adopt_sym ($2, TOK_POS); }
        | '-' expr %prec TOK_UNI  { $$ = $1->adopt_sym ($2, TOK_NEG); }
        | '!' expr %prec TOK_UNI  { $$ = $1->adopt($2); }
        | allocator             { $$ = $1; }
        | call                  { $$ = $1; }
        | '(' expr ')' { destroy ($1, $3); $$ = $2; }
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
    | TOK_IDENT '(' call_args ')'
        {
            destroy($4);
            $2 = $2->adopt($3);
            $$ = $1->adopt($2);
        }
    ;

call_args
    : call_args ',' expr
        {
            destroy($2);
            $$ = $1->adopt($3);
        }
    | expr
    ;

variable
    : TOK_IDENT
    | expr '[' expr ']'
        {
            destroy($4);
            $2->symbol = TOK_INDEX;
            $$ = $2->adopt($1, $3);
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
