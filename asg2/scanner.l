/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int symbol) {
    yylval = new astree (symbol, lexer::lloc, yytext);
    fprintf(token_file, "%zd %2zd.%.03zd  %-3d  %-15s (%s)\n",
       lexer::lloc.filenr, lexer::lloc.linenr,
       lexer::lloc.offset, symbol, parser::get_tname(symbol), yytext);
    return symbol;
}

%}

%option 8bit
%option debug
%option nodefault
%option noinput
%option nounput
%option noyywrap
%option warn
%option verbose

LETTER          [A-Za-z_]
DIGIT           [0-9]
INTCONST        ({DIGIT}{DIGIT}*)
CHARCONST       ('([^\\'\n]|\\[\\'"0nt])')
STRINGCONST     (\"([^\\"\n]|\\[\\'"0nt])*\")
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
INVIDENT        ({DIGIT}({LETTER}|{DIGIT})*)
INVSTRINGCONST  ((\"[^\"]*)|(\".*\\[^\\'"nt0].*\"))
INVCHARCONST    ('[^\n']*)




%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }




"if"            { return yylval_token (TOK_IF); }
"else"          { return yylval_token (TOK_ELSE); }
"while"         { return yylval_token (TOK_WHILE); }
"return"        { return yylval_token (TOK_RETURN); }
"int"           { return yylval_token (TOK_INT); }
"string"        { return yylval_token (TOK_STRING); }
"struct"        { return yylval_token (TOK_STRUCT); }
"void"          { return yylval_token (TOK_VOID); }
"new"           { return yylval_token (TOK_NEW); }
"null"          { return yylval_token (TOK_NULL); }


"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }
"="             { return yylval_token ('='); }
"=="            { return yylval_token (TOK_EQ); }
"!="            { return yylval_token (TOK_NE); }
"<"             { return yylval_token ('<'); }
"<="            { return yylval_token (TOK_LE); }
">"             { return yylval_token ('>'); }
">="            { return yylval_token (TOK_GE); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
";"             { return yylval_token (';'); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }
"[]"            { return yylval_token (TOK_NEWARRAY); }

{INTCONST}          { return yylval_token (TOK_INTCON); }
{CHARCONST}         { return yylval_token (TOK_CHARCON); }
{STRINGCONST}       { return yylval_token (TOK_STRINGCON); }
{IDENT}             { return yylval_token (TOK_IDENT); }


{INVIDENT}      { lexer::badtoken (yytext); }
{INVSTRINGCONST}      { lexer::badtoken (yytext); }
{INVCHARCONST}      { lexer::badtoken (yytext); }
.               { lexer::badchar (*yytext); }


%%
