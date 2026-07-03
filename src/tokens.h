#ifndef TOKENS_H_
#define TOKENS_H_

typedef struct Tok {
        long token;
        union {
                double real_number;
                long int_number;
                char *string;
        };
} Tok;

typedef enum {
        TOK_INT = 256,
        TOK_REAL,
        TOK_STR,
        TOK_PATH,
        TOK_EOF,
} Tok_Type;

/*
{INT} { return (Tok) { .int_number = atoi(yytext), .type=TOK_INT }; }
{REAL} { return (Tok) { .real_number = strtod(yytext, 0), .type=TOK_REAL }; }
{PATH} { return (Tok) { .string = strdup(yytext), .type=TOK_PATH }; }
{STR} { return (Tok) { .string = strdup(yytext), .type=TOK_STR }; }
")" { return (Tok) { .string = strdup(yytext), .type=TOK_RPAR }; }
"(" { return (Tok) { .string = strdup(yytext), .type=TOK_LPAR }; }
*/

#endif // !TOKENS_H_
