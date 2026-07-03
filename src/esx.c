#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "da.h"
#include "esx.h"
#include "lexer.h"
#include "tokens.h"

static char *
fix_str(char *str)
{
        // remove doble quotes
        char *s = strdup(str + 1);
        s[strlen(s) - 1] = 0;

        // escape escaped chars
        for (int i = 0; s[i] && s[i + 1]; i++) {
                if (s[i] != '\\') continue;
                memmove(&s[i], &s[i + 1], strlen(&s[i + 1]) + 1);
                switch (s[i]) {
                case 'n':
                        s[i] = '\n';
                        /* ... */
                }
        }
        return s;
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
lexer_get_token(Tok *tok)
{
        int t = yylex();
        tok->token = t;
        switch (t) {
        case TOK_EOF: return 0;
        case TOK_INT: {
                int base = 10;
                if (!strncmp(yytext, "0x", 2)) base = 16;
                if (!strncmp(yytext, "0o", 2)) base = 8;
                tok->int_number = strtol(yytext, NULL, base);
        } break;

        case TOK_REAL: tok->real_number = strtod(yytext, NULL); break;
        case TOK_STR: tok->string = fix_str(yytext); break;
        case TOK_PATH: tok->string = strdup(yytext); break;
        case '(': tok->string = "("; break;
        case ')': tok->string = ")"; break;
        case '|': tok->string = "|"; break;

        default:
                printf("Invalid switch case %d\n", t);
                exit(1);
        }
        return t;
}

static int
parse_tokens(Tok *lex, char *src, Esx_Program *prog)
{
        int has_error = 0;

        while (lexer_get_token(lex)) {
                int token = lex->token;
                switch (token) {
                case TOK_EOF: break;
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
                case TOK_INT: token_append(prog, (Esx_Token) { .type = Esx_Intlit, .as.i = lex->int_number }); break;
                case TOK_REAL: token_append(prog, (Esx_Token) { .type = Esx_FloatLit, .as.f = lex->real_number }); break;
                case TOK_STR: token_append(prog, (Esx_Token) { .type = Esx_String, .as.s = strdup(lex->string) }); break;
                case TOK_PATH: token_append(prog, (Esx_Token) { .type = Esx_Atom, .as.s = strdup(lex->string) }); break;
                        // clang-format on

                default:
                        printf("Error: unexpected token\n");
                        has_error = 1;
                        break;
                }
        while_continue:;
        }
        return has_error;
}

int
esx_parse_string(char *buf, ssize_t buflen, Esx_Program *prog)
{
        memset(prog, 0, sizeof(Esx_Program));

        if (buf[buflen] != 0) {
                printf("Buffer len is not valid\n");
                abort();
        }

        Tok tok = { 0 };

        printf("ESX: parse string (`%.*s`)\n", (int) buflen, buf);

        YY_BUFFER_STATE bufferState = yy_scan_string(buf);
        int s = parse_tokens(&tok, buf, prog);
        yy_delete_buffer(bufferState);
        yylex_destroy();
        return s;
}

int
esx_parse_file(const char *filename, Esx_Program *prog)
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
        case Esx_String: snprintf(buf, size, "%s", tok.as.s); break;
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
