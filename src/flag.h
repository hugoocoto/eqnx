/* Copyright (c) 2026 Hugo Coto */

/* This is not tested at all. It works in the few test I did but it can fail */

/* Todo:
 * - accept -a=AA
 * - accept positional arguments? maybe not, I mean, you can iter over argv
 */

#ifndef FLAG2_H_
#define FLAG2_H_ 1

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FLAG_COUNT 3

struct flag_opts {
        char *opt;      // Flag (--help)
        char *abbr;     // Flag abbreviation (-h)
        char *help;     // Help message for the flag
        int nargs;      // Number of args to catch (max 1)
        char *defaults; // Default value as string (default is a keyword)
        int required;   // Set to 1 if the flag must be set
        char **var;     // Stores the pointer to the variable where the value
                        // should be set
};

static struct program_opts {
        char *name;
        char *help;
} flag_prog = { 0 };


static struct {
        int count;
        struct flag_opts flags[MAX_FLAG_COUNT];
} flag_flags = {
        .count = 1,
        .flags = { (struct flag_opts) {
        .opt = (char *) "--help", // the cast is to avoid warnings
        .abbr = (char *) "-h",
        .help = (char *) "Show this help",
        } }
};

#define flag_add(var, ...) __flag_add(var, (struct flag_opts) { __VA_ARGS__ })
#define flag_program(...) __flag_program((struct program_opts) { __VA_ARGS__ })


#define YOU_SHOULD_NOT_USE_THIS_FUNCTION

static void
flag_show_help(int fileno)
{
        int i, j;

        dprintf(fileno, "usage: %s", flag_prog.name);
        if (flag_flags.count == 0) goto prog_help;

        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, flag_flags.flags[i].required ? " " : " [");
                if (flag_flags.flags[i].abbr)
                        dprintf(fileno, "%s", flag_flags.flags[i].abbr);
                else
                        dprintf(fileno, "%s", flag_flags.flags[i].opt);
                for (j = 0; j < flag_flags.flags[i].nargs; j++)
                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[2]));
                dprintf(fileno, flag_flags.flags[i].required ? "" : "]");
        }

prog_help:
        dprintf(fileno, "\n\n");
        if (flag_prog.help) dprintf(fileno, "%s\n\n", flag_prog.help);
        if (flag_flags.count == 0) return;

        dprintf(fileno, "options:\n");
        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, " ");
                if (flag_flags.flags[i].opt)
                        dprintf(fileno, "%s", flag_flags.flags[i].opt);
                if (flag_flags.flags[i].abbr)
                        dprintf(fileno, ", %s", flag_flags.flags[i].abbr);
                for (j = 0; j < flag_flags.flags[i].nargs; j++)
                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[2]));
                dprintf(fileno, " \t");
                if (flag_flags.flags[i].help)
                        dprintf(fileno, "%s", flag_flags.flags[i].help);
                if (flag_flags.flags[i].defaults)
                        dprintf(fileno, " (default: %s)", flag_flags.flags[i].defaults);
                dprintf(fileno, "\n");
        }
}

YOU_SHOULD_NOT_USE_THIS_FUNCTION static void
__flag_add(char **var, struct flag_opts opts)
{
        if (flag_flags.count == MAX_FLAG_COUNT) {
                fprintf(stderr, "Max flag count reached!"
                                " Change it in " __FILE__ "\n");
                exit(3);
        }
        if ((opts.var = var)) *opts.var = NULL;
        flag_flags.flags[flag_flags.count] = opts;
        ++flag_flags.count;
}

YOU_SHOULD_NOT_USE_THIS_FUNCTION static void
__flag_program(struct program_opts opts)
{
        flag_prog = opts;
}

static int
flag_parse(int *argc, char ***argv)
{
        struct flag_opts *fopt;
        int i, j;

        if (!flag_prog.name || !*flag_prog.name) flag_prog.name = **argv;

        for (i = 0; i < *argc; i++) {
                if (strcmp(argv[0][i], "-h") == 0 ||
                    strcmp(argv[0][i], "-help") == 0 ||
                    strcmp(argv[0][i], "--help") == 0) {
                        flag_show_help(STDOUT_FILENO);
                        exit(0);
                }
        }

        for (i = 0; i < *argc; i++) {
                for (j = 0; j < flag_flags.count; j++) {
                        fopt = flag_flags.flags + j;
                        if (!fopt->var) continue;

                        int o = fopt->opt &&
                                !strncmp(fopt->opt, argv[0][i], strlen(fopt->opt)) &&
                                (argv[0][i][strlen(fopt->opt)] == 0 ||
                                 argv[0][i][strlen(fopt->opt)] == '=');
                        int a = fopt->abbr &&
                                !strncmp(fopt->abbr, argv[0][i], strlen(fopt->abbr)) &&
                                (argv[0][i][strlen(fopt->abbr)] == 0 ||
                                 argv[0][i][strlen(fopt->abbr)] == '=');

                        /* var already set or name not match */
                        if (!*fopt->var && !o && !a) continue;

                        if (fopt->nargs > 0) {
                                if (fopt->nargs > 1) {
                                        fprintf(stderr, "Ups! Unsupported nargs > 1\n");
                                        return 1;
                                }
                                if ((o && argv[0][i][strlen(fopt->opt)] == '=') ||
                                    (a && argv[0][i][strlen(fopt->abbr)] == '=')) {
                                        *fopt->var = strchr(argv[0][i], '=') + 1;
                                } else {
                                        *fopt->var = argv[0][i + 1];
                                        ++i;
                                }
                        } else
                                *fopt->var = (char *) 1;
                }
        }

        for (j = 0; j < flag_flags.count; j++) {
                fopt = flag_flags.flags + j;
                if (fopt->var == NULL || *fopt->var != NULL) continue;
                if (fopt->defaults) *fopt->var = fopt->defaults;
                if (fopt->required && *fopt->var == NULL) {
                        fprintf(stderr, "Required flag %s not set!\n",
                                fopt->opt  ?:
                                fopt->abbr ?:
                                             "??");
                        return 2;
                }
        }
        return 0;
}

#endif
