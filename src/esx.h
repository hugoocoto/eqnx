#ifndef ESX_H_
#define ESX_H_

#include <stdbool.h>

typedef struct Esx_Token Esx_Token;
typedef Esx_Token Esx_Program;

extern int esx_to_args(Esx_Program program, int *e_argc, char ***e_argv);
extern int esx_parse_file(char *filename, Esx_Program *prog);
extern int esx_parse_string(char *buf, long buflen, Esx_Program *prog);

#define TYPE_LIST          \
        TYPE(Esx_Atom)     \
        TYPE(Esx_String)   \
        TYPE(Esx_Intlit)   \
        TYPE(Esx_FloatLit) \
        TYPE(Esx_CharLit)  \
        TYPE(Esx_Expr)

typedef struct Esx_Token {
        enum {
/*           */ #define TYPE(x) x,
                TYPE_LIST
/*           */ #undef TYPE
        } type;
        struct Esx_Token *next;
        union {
                char *s; // string
                int c;   // char
                int i;   // int
                float f; // float
                Esx_Program *expr;
        } as;
} Esx_Token;

// type, name, default value
#define Esx_Print_Opts_List          \
        EPOL(bool, pretty, true)     \
        EPOL(const char *, sep, "|") \
        EPOL(void *, file, stdout)   \
        EPOL(char *, end, "\n")

typedef struct Esx_Print_Opts {
/*   */ #define EPOL(t, v, d) t v;
        Esx_Print_Opts_List
/*   */ #undef EPOL
} Esx_Print_Opts;

// Todo: Fix this

#define EPOL(t, v, d) .v = (d),

#define esx_print_program(prog, ...) \
        if ((prog).next || (assert(!"prognext"), 0)) __esx_print_expression((prog).next->as.expr->next, (Esx_Print_Opts) { Esx_Print_Opts_List __VA_ARGS__ });

#define esx_print_expression(expr, ...) \
        __esx_print_expression((expr), (Esx_Print_Opts) { Esx_Print_Opts_List __VA_ARGS__ });

#define esx_print_single_token(tok, ...) \
        esx_print_token((tok), 0, (Esx_Print_Opts) { Esx_Print_Opts_List __VA_ARGS__ });

void __esx_print_expression(Esx_Token *tok, Esx_Print_Opts opts);
int esx_print_token(Esx_Token *last, int offset, Esx_Print_Opts opts);

#endif
