/* $Id: lexer.l,v 1.7 2016-10-27 19:45:59-07 - - $ */

%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int symbol) {
   yylval = new astree (symbol, lexer::lloc, yytext);
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
INTCONST		({DIGIT}{DIGIT}*)
CHARCONST		(’([^\\’\n]|\\[\\’"0nt])’)
STRINGCONST		(\"([^\\"\n]|\\[\\’"0nt])*\")
RESERVED		(if|else|while|return|int|string|struct|void|new|null)
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
INVIDENT		({DIGIT}({LETTER}|{DIGIT})*)


/* TODO

INVSTRINGCONST  (\"(([^\\"\n]|\\[\\’"0nt])*)|\")
INVCHARCONST    (’([^\\’\n]|\\[\\’"0nt])|')


*/

%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }


{RESERVED}		{ return yylval_token (RESERVED); }
{INTCONST}        { return yylval_token (INTCONST); }
{CHARCONST}		{ return yylval_token (CHARCONST); }
{STRINGCONST}		{ return yylval_token (STRINGCONST); }
{IDENT}         { return yylval_token (IDENT); }


"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }
"="             { return yylval_token ('='); }
"=="            { return yylval_token ('=='); }
"!="            { return yylval_token ('!='); }
"<"             { return yylval_token ('<'); }
"<="            { return yylval_token ('<='); }
">"             { return yylval_token ('>'); }
">="            { return yylval_token ('>='); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
";"             { return yylval_token (';'); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }
"[]"            { return yylval_token ('[]'); }
/*
{NOTNUMBER}     { lexer::badtoken (yytext);
                  return yylval_token (NUMBER); }
*/
.               { lexer::badchar (*yytext); }
{INVIDENT}		{ lexer::badtoken (yytext); }

%%

