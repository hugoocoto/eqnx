#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STB_C_LEXER_IMPLEMENTATION
#include "../thirdparty/stb_c_lexer.h"

#include "da.h"
#include "esx.h"

const char *
clex_repr(int token)
{
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

static void
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

static int
parse_tokens(stb_lexer *lex, char *src, Esx_Program *prog)
{
        int has_error = 0;

        while (stb_c_lexer_get_token(lex)) {
                int token = lex->token;
                switch (token) {
                case CLEX_parse_error: return 1;
                case '|': goto while_continue;

                case '(': {
                        Esx_Program *expr = calloc(1, sizeof(Esx_Program));
                        parse_tokens(lex, src, expr);
                        token_append(prog, (Esx_Token) { .type = Esx_Expr, .as.expr = expr });
                        break;
                }
                case ')':
                        return has_error;

                        // clang-format off
                case CLEX_intlit: token_append(prog, (Esx_Token) { .type = Esx_Intlit, .as.i = lex->int_number }); break;
                case CLEX_floatlit: token_append(prog, (Esx_Token) { .type = Esx_FloatLit, .as.f = lex->real_number }); break;
                case CLEX_charlit: token_append(prog, (Esx_Token) { .type = Esx_CharLit, .as.i = lex->int_number }); break;
                case CLEX_dqstring:
                case CLEX_sqstring: token_append(prog, (Esx_Token) { .type = Esx_String, .as.s = strdup(lex->string) }); break;
                case CLEX_id: token_append(prog, (Esx_Token) { .type = Esx_Atom, .as.s = strdup(lex->string) }); break;
                        // clang-format on

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
        while_continue:;
        }
        return has_error;
}

int
esx_parse_string(char *buf, ssize_t buflen, Esx_Program *prog)
{
/*   */ #define pool_size 4 * 1024

        memset(prog, 0, sizeof(Esx_Program));

        if (buf[buflen] != 0) {
                printf("Buffer len is not valid");
                abort();
        }

        char *pool = malloc(pool_size);
        assert(pool);
        stb_lexer lexer = { 0 };
        stb_c_lexer_init(&lexer, buf, buf + buflen, pool, pool_size);
        return parse_tokens(&lexer, buf, prog);
}

int
esx_parse_file(char *filename, Esx_Program *prog)
{
        char buf[4 * 1024] = { 0 };
        ssize_t n;
        int fd = open(filename, O_RDONLY);

        int should_break = false;
        if (fd < 0) should_break = true;
        n = fd >= 0 ? read(fd, buf, sizeof buf - 1) : fd;
        if (n <= 0) {
                printf("Failed to read file %s\n", filename);
                return 1;
        }
        int len = strlen(buf);
        if (len != n) {
                printf("len %d != n %zd\n", len, n);
                abort();
        }
        printf("%s open: %zd bytes\n", filename, n);
        assert(should_break == false);
        return esx_parse_string(buf, n, prog);
}

static char *
type_repr(Esx_Token tok, char *buf, int size)
{
        static char *const lookup[] = {
/*           */ #define TYPE(x) [x] = #x,
                TYPE_LIST
/*           */ #undef TYPE
        };
        return strncpy(buf, lookup[tok.type], size);
}

static char *
value_repr(Esx_Token tok, char *buf, int size)
{
        switch (tok.type) {
        case Esx_Atom: snprintf(buf, size, "%s", tok.as.s); break;
        case Esx_String: snprintf(buf, size, "\"%s\"", tok.as.s); break;
        case Esx_Intlit: snprintf(buf, size, "%lu", tok.as.i); break;
        case Esx_FloatLit: snprintf(buf, size, "%f", tok.as.f); break;
        case Esx_CharLit: snprintf(buf, size, "'%c'", tok.as.c); break;
        default: {
                char name[32] = { 0 };
                type_repr(tok, name, sizeof name - 1);
                snprintf(buf, size, "err `%s` has no value repr", name);
        } break;
        }
        return buf;
}

int
esx_print_token(Esx_Token *last, int offset, Esx_Print_Opts opts)
{
        char value[64];
        int has_childs = 0;
        switch (last->type) {
        case Esx_Atom:
                fprintf(opts.file, "%s", value_repr(*last, value, sizeof value - 1));
                break;

        case Esx_Expr:
                has_childs = true;
                __esx_print_expression(last->as.expr->next, opts);
                break;

        case Esx_String:
        case Esx_Intlit:
        case Esx_FloatLit:
        case Esx_CharLit:
                fprintf(opts.file, "%s", value_repr(*last, value, sizeof value - 1));
                break;

        default:
                if (opts.pretty) fprintf(opts.file, "%*.*s", offset, offset, "");
                fprintf(opts.file, "%s?", opts.sep);
        }
        return has_childs;
}

void
__esx_print_expression(Esx_Token *tok, Esx_Print_Opts opts)
{
/*   */ #define OFFSET 4
        static int offset = -OFFSET;
        Esx_Token *last = tok;
        bool has_childs = false;

        if (!tok) return;

        offset += OFFSET;
        if (offset > 0 && opts.pretty) {
                fprintf(opts.file, "\n");
                fprintf(opts.file, "%*.*s", offset, offset, "");
        }
        fprintf(opts.file, "(");

        while (last) {
                switch (last->type) {
                case Esx_String:
                case Esx_Intlit:
                case Esx_FloatLit:
                case Esx_CharLit: fprintf(opts.file, "%s", opts.sep); break;
                case Esx_Atom:
                case Esx_Expr: break;
                }

                has_childs = esx_print_token(last, offset, opts) ?: has_childs;
                last = last->next;
        }
        if (has_childs && opts.pretty) {
                fprintf(opts.file, "\n");
                fprintf(opts.file, "%*.*s", offset, offset, "");
        }
        fprintf(opts.file, ")");
        offset -= OFFSET;
        if (offset < 0) fprintf(opts.file, "%s", opts.end);
}

int
esx_to_args(Esx_Program program, int *e_argc, char ***e_argv)
{
        struct {
                int capacity;
                int count;
                char **data;
        } args = { 0 };
        FILE *f = tmpfile();
        assert(f);
        char *buf;

        *e_argc = 0;
        *e_argv = NULL;
        if (!program.next) return 0;

        assert(program.next->type == Esx_Expr);
        assert(program.next->as.expr->next->type == Esx_Atom);

        Esx_Token *last = program.next->as.expr;

        while ((last = last->next)) {
                fseek(f, 0, SEEK_SET);
                esx_print_single_token(last, .sep = "|", .pretty = false, .file = f, .end = " ");
                size_t size = ftell(f);
                buf = calloc(1, size + 1);
                assert(buf);
                assert(fseek(f, 0, SEEK_SET) == 0);
                ssize_t n = fread(buf, 1, size, f);
                assert((size_t) n == size);
                da_append(&args, buf);
        }

        da_append(&args, 0); // null terminate argv
        *e_argc = args.count;
        *e_argv = args.data;

        return 0;
}
