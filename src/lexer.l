%option noyywrap noinput nounput

%{
#include "tokens.h"
%}

/*
TOK_INT,
TOK_REAL,
TOK_STR,
TOK_PATH,
TOK_LPAR,
TOK_RPAR,
*/

STRCHR [^\n"\\]
ESCSEQ [\\].

INT [0-9]+
OCT "0o"[0-7]+
HEX "0x"[0-9a-fA-F]+
REAL ({INT}"."{INT}?)|("."{INT})
STR "\""({STRCHR}*|{ESCSEQ})*"\""
PATH [^ \n()0-9][^ \n()]*
WS [ \n\v\f\t\r]+

%%

{WS}    { }
")"     { return ')'; }
"("     { return '('; }
"|"     { return '|'; }
{INT}   { return TOK_INT; }
{OCT}   { return TOK_INT; }
{HEX}   { return TOK_INT; }
{REAL}  { return TOK_REAL; }
{STR}   { return TOK_STR; }
{PATH}  { return TOK_PATH; }
<<EOF>> { return TOK_EOF; }
. {
    printf("Unknown token '%s'\n", yytext);
}

%%
