#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STB_C_LEXER_IMPLEMENTATION
#include "../thirdparty/stb_c_lexer.h"

typedef struct Esx_Token Esx_Token;
typedef Esx_Token Esx_Program;

#define TYPE_LIST      \
        TYPE(Esx_Atom) \
        TYPE(Esx_Expr)

typedef struct Esx_Token {
        enum {
/*           */ #define TYPE(x) x,
                TYPE_LIST
/*           */ #undef TYPE
        } type;
        struct Esx_Token *next;
        union {
                char *string;
                Esx_Program *expr;
        } as;
} Esx_Token;

const char *
clex_repr(int token)
{
        static char *ascii[256];
        static const char *const lookup[] = {
                [CLEX_eof] = "CLEX_eof",
                [CLEX_parse_error] = "CLEX_parse_error",
                [CLEX_intlit] = "CLEX_intlit",
                [CLEX_floatlit] = "CLEX_floatlit",
                [CLEX_id] = "CLEX_id",
                [CLEX_dqstring] = "CLEX_dqstring",
                [CLEX_sqstring] = "CLEX_sqstring",
                [CLEX_charlit] = "CLEX_charlit",
                [CLEX_eq] = "CLEX_eq",
                [CLEX_noteq] = "CLEX_noteq",
                [CLEX_lesseq] = "CLEX_lesseq",
                [CLEX_greatereq] = "CLEX_greatereq",
                [CLEX_andand] = "CLEX_andand",
                [CLEX_oror] = "CLEX_oror",
                [CLEX_shl] = "CLEX_shl",
                [CLEX_shr] = "CLEX_shr",
                [CLEX_plusplus] = "CLEX_plusplus",
                [CLEX_minusminus] = "CLEX_minusminus",
                [CLEX_pluseq] = "CLEX_pluseq",
                [CLEX_minuseq] = "CLEX_minuseq",
                [CLEX_muleq] = "CLEX_muleq",
                [CLEX_diveq] = "CLEX_diveq",
                [CLEX_modeq] = "CLEX_modeq",
                [CLEX_andeq] = "CLEX_andeq",
                [CLEX_oreq] = "CLEX_oreq",
                [CLEX_xoreq] = "CLEX_xoreq",
                [CLEX_arrow] = "CLEX_arrow",
                [CLEX_eqarrow] = "CLEX_eqarrow",
                [CLEX_shleq] = "CLEX_shleq",
                [CLEX_shreq] = "CLEX_shreq",
                [CLEX_first_unused_token] = "CLEX_first_unused_token",
        };

        if (token > 0 && token >= CLEX_eof && token <= CLEX_first_unused_token) {
                return lookup[token];
        }
        return NULL;
}

void
token_append(Esx_Program *prog, Esx_Token token)
{
        assert(prog);
        Esx_Token *last = prog;
        while (last->next) {
                last = last->next;
        }
        last->next = malloc(sizeof token);
        assert(last->next);
        *last->next = token;
}

int esx_parse_tokens(stb_lexer *lexer, char *src, Esx_Program *prog);
int esx_expect_token(stb_lexer *lex, int tok);

Esx_Token *
esx_get_consume_expr(stb_lexer *lex, char *src, Esx_Program *prog)
{
        if (esx_expect_token(lex, '(')) return NULL;
        Esx_Program p;
        esx_parse_tokens(lex, src, prog);
        stb_c_lexer_get_token(lex); // get consume ')'
        if (esx_expect_token(lex, ')')) return NULL;
        return p.next;
}

int
esx_expect_token(stb_lexer *lex, int tok)
{
        if (lex->token == tok) {
                return 0;
        }

        stb_lex_location loc;
        stb_c_lexer_get_location(lex, lex->where_firstchar, &loc);
        printf("Error: %d:%d expected ", loc.line_number, loc.line_offset + 1);
        if (0 < tok && tok < 256) {
                printf("`%c`", tok);
        } else {
                printf("%s", clex_repr(tok));
        }
        printf(" but ");
        if (0 < lex->token && lex->token < 256) {
                printf("`%c`", (char) lex->token);
        } else {
                printf("%s", clex_repr(lex->token));
        }
        printf(" found\n");
        return 1;
}

int
esx_parse_tokens(stb_lexer *lex, char *src, Esx_Program *prog)
{
        int has_error = 0;

        while (stb_c_lexer_get_token(lex)) {
                int token = lex->token;
                switch (token) {
                case CLEX_parse_error: return 1;
                case '(': {
                        Esx_Program *expr = malloc(sizeof *expr);
                        esx_parse_tokens(lex, src, expr);
                        token_append(prog, (Esx_Token) {
                                           .type = Esx_Expr,
                                           .as.expr = expr,
                                           });
                        break;
                }
                case ')': return has_error;

                case CLEX_id:
                        token_append(prog, (Esx_Token) {
                                           .type = Esx_Atom,
                                           .as.string = strdup(lex->string),
                                           });
                        break;

                default: {
                        stb_lex_location loc;
                        stb_c_lexer_get_location(lex, lex->where_firstchar, &loc);
                        printf("Error: %d:%d unexpected token ", loc.line_number, loc.line_offset + 1);
                        if (0 < token && token < 256) {
                                printf("`%c`\n", token);
                        } else {
                                printf("%s\n", clex_repr(token));
                        }
                        has_error = 1;
                } break;
                }
        }
        return has_error;
}

int
esx_parse_string(char *buf, ssize_t buflen, Esx_Program *prog)
{
/*   */ #define pool_size 4 * 1024
        char *pool = malloc(pool_size);
        stb_lexer lexer;
        stb_c_lexer_init(&lexer, buf, buf + buflen, pool, pool_size);
        int status = esx_parse_tokens(&lexer, buf, prog);
        return status;
}

int
esx_parse_file(char *filename, Esx_Program *prog)
{
        char buf[4 * 1024] = { 0 };
        ssize_t n;
        int fd = open(filename, O_RDONLY);

        if (fd < 0) {
                printf("Failed to open file %s\n", filename);
                return 1;
        }

        n = read(fd, buf, sizeof buf - 1);

        if (n <= 0) {
                printf("Failed to read file %s\n", filename);
                return 1;
        }

        return esx_parse_string(buf, n, prog);
}

char *
esx_type_repr(Esx_Token tok, char *buf, int size)
{
        static char *const lookup[] = {
/*           */ #define TYPE(x) [x] = #x,
                TYPE_LIST
/*           */ #undef TYPE
        };
        return strncpy(buf, lookup[tok.type], size);
}

void esx_print_program(Esx_Program prog);

char *
esx_value_repr(Esx_Token tok, char *buf, int size)
{
        switch (tok.type) {
        case Esx_Atom:
                snprintf(buf, size, "%s", tok.as.string);
                break;

        default: {
                char name[16] = { 0 };
                esx_type_repr(tok, name, sizeof name - 1);
                snprintf(buf, size, "type %s has no value repr", name);
        } break;
        }
        return buf;
}

void
esx_print_program(Esx_Program prog)
{
        Esx_Token *last = prog.next;
        char name[16];
        char value[16];
        while (last) {
                switch (last->type) {
                case Esx_Atom:
                        printf("[%s] %s\n",
                               esx_type_repr(*last, name, sizeof name - 1),
                               esx_value_repr(*last, value, sizeof value - 1));
                        break;
                case Esx_Expr:
                        esx_print_program(*last->as.expr);
                }
                last = last->next;
        }
}

int
main(int argc, char **argv)
{
        if (argc != 2) {
                printf("Invalid arguments\n");
                exit(1);
        }

        Esx_Program program;

        if (esx_parse_file(argv[1], &program)) {
                printf("Can not parse file %s\n", argv[1]);
                exit(1);
        }

        esx_print_program(program);


        return 0;
}
