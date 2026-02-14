#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <time.h>
#include <unistd.h>

#include "../thirdparty/toml-c.h"
#undef calloc
#undef strdup
#undef strndup

#include "../src/plug_api.h"
#include "da.h"
#include "theme.h"

typedef struct Month Month;

Window *self_window = NULL;
Plugin *self_plugin = NULL;
Month *active_month = NULL;

static int month_cell_height;
static int month_cell_width;
static int cursor = 0;

typedef struct Cell {
        struct tm tm;
        time_t time;
} Day;

typedef struct Month {
        Day days[32];
        int len;
} Month;

typedef struct Task {
        char *name;
        char *desc;
        time_t date;
        uint32_t fg, bg;
} Task;

struct Task_DA {
        int capacity;
        int size;
        Task *data;
} tasks = { 0 };

/* List of constant predefined tasks */
#define LIST_OF_TASKS \
        T("now", 0, time(0), BACKGROUND, YELLOW)


/* Month [0, 11] (January = 0) */
Month *
month_open(int month, int year)
{
        assert(year >= 1900);
        Month *m = calloc(1, sizeof(Month));
        assert(m);
        for (int i = 0;; ++i) {
                assert(i <= 31);
                m->days[i].tm = (struct tm) {
                        .tm_sec = 0,            /* Seconds          [0, 60] */
                        .tm_min = 0,            /* Minutes          [0, 59] */
                        .tm_hour = 0,           /* Hour             [0, 23] */
                        .tm_mday = i + 1,       /* Day of the month [1, 31] */
                        .tm_mon = month,        /* Month            [0, 11]  (January = 0) */
                        .tm_year = year - 1900, /* Year minus 1900 */
                        .tm_isdst = -1,         /* Daylight savings flag */
                };
                assert((m->days[i].time = mktime(&m->days[i].tm)) > 0);
                if (m->days[i].tm.tm_mon != month) {
                        m->len = i;
                        break;
                }
        }
        return m;
}

void
month_close(Month *m)
{
        if (m) free(m);
}

Month *
get_current_month()
{
        time_t t = time(NULL);
        assert(t > 0);
        struct tm *tm = localtime(&t);
        return month_open(tm->tm_mon, tm->tm_year + 1900);
}

Month *
get_next_month(Month *m)
{
        printf("getting next month\n");
        struct tm tm = m->days[0].tm;
        int mon = (tm.tm_mon + 1) % 12;
        int year = tm.tm_year + (tm.tm_mon == 11);
        return month_open(mon, year + 1900);
}

Month *
get_prev_month(Month *m)
{
        printf("getting prev month\n");
        struct tm tm = m->days[0].tm;
        int mon = (tm.tm_mon + 11) % 12;
        int year = tm.tm_year - (tm.tm_mon == 0);
        return month_open(mon, year + 1900);
}

void
resize(int px, int py, int pw, int ph)
{
        int w = 0, h = 0;
        window_px_to_coords(pw, ph, &w, &h);
        month_cell_width = w / 7;
        month_cell_height = (h - 1) / 6;
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        if (sym == 'n' && mods | Mod_Control) {
                active_month = get_next_month(active_month);
                cursor = 0;
                ask_for_redraw();
        }

        else if (sym == 'p' && mods | Mod_Control) {
                active_month = get_prev_month(active_month);
                cursor = 0;
                ask_for_redraw();
        }

        else if (sym == 'h' && mods == Mod_None) {
                if (cursor > 0) --cursor;
                ask_for_redraw();
        } else if (sym == 'j' && mods == Mod_None) {
                ask_for_redraw();
                if ((cursor += 7) >= active_month->len) cursor = active_month->len - 1;
        } else if (sym == 'k' && mods == Mod_None) {
                if ((cursor -= 7) < 0) cursor = 0;
                ask_for_redraw();
        } else if (sym == 'l' && mods == Mod_None) {
                if (cursor < active_month->len - 1) ++cursor;
                ask_for_redraw();
        }
}

// To be implemented
// void
// pointer_event(Pointer_Event e)
// {
// }

const char *const numbers[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
        "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25",
        "26", "27", "28", "29", "30", "31"
};

const char *
repr_month(Month *m)
{
        static char buf[128];
        memset(buf, 0, sizeof buf);
        strftime(buf, sizeof buf - 1, "%B %Y", &m->days->tm);
        return buf;
}

const char *
repr_timet(time_t timer)
{
        static char buf[128];
        memset(buf, 0, sizeof buf);
        strftime(buf, sizeof buf - 1, "%F %T", localtime(&timer));
        return buf;
}

Task
get_task_between(time_t t0, time_t t1, int index)
{
#define USE_CACHE /* toggleable as I don't know if it works (I know I have to \
                     use a sorted data structure but I didn't have time yet) */
        int i = 0;
#ifdef USE_CACHE
        static struct {
                time_t t0;
                time_t t1;
                time_t index;
                time_t da_index;
        } cache = { 0 };

        int start_index = index;
        if (cache.t0 == t0 && cache.t1 == t1 && cache.index <= index) {
                i = cache.da_index;
                index -= cache.index;
        }
#endif

        for (; i < tasks.size; i++) {
                if (tasks.data[i].date >= t0 && tasks.data[i].date < t1) {
                        if (index == 0) {
#ifdef USE_CACHE
                                cache.t0 = t0;
                                cache.t1 = t1;
                                cache.da_index = i;
                                cache.index = start_index;
#endif
                                return tasks.data[i];
                        }
                        --index;
                }
        }
        return (Task) { 0 };
}

void
render_month()
{
        if (month_cell_height < 2 || month_cell_width < 3) {
                window_clear(self_window, RED, RED);
                window_puts(self_window, 0, 0, FOREGROUND, RED, "Screen too small");
                return;
        }

        int offset = active_month->days[0].tm.tm_wday;
        assert(offset >= 0 && offset <= 6 * month_cell_width);
        uint32_t fg, bg, asc_bg, asc_fg;
        int n = 0, i, j, k;

        window_printf(self_window, 0, 0, FOREGROUND, BACKGROUND, "%s", repr_month(active_month));
        for (j = 0;; j += month_cell_height) {
                for (i = offset; i < 7 && n < active_month->len; i++, n++, offset = 0) {
                        assert(n >= 0 && n <= 30);

                        if (cursor != n) {
                                fg = FOREGROUND;
                                bg = GRAY;
                                asc_fg = BLUE;
                                asc_bg = BACKGROUND;
                        } else {
                                fg = BLACK;
                                bg = WHITE;
                                asc_fg = WHITE;
                                asc_bg = BACKGROUND;
                        }

                        window_printf(self_window, i * month_cell_width + 1, j + 1, asc_fg, asc_bg, "%*.*s ",
                                      month_cell_width - 2, month_cell_width - 2, numbers[n]);
                        for (k = 1; k < month_cell_height; k++) {
                                time_t t0 = active_month->days[n].time;
                                time_t t1 = active_month->days[n + 1].time;
                                Task t = get_task_between(t0, t1, k - 1);
                                window_printf(self_window, i * month_cell_width + 1, j + k + 1, t.fg ?: fg, t.bg ?: bg, "%-*.*s",
                                              month_cell_width - 1, month_cell_width - 1, t.name ?: "");
                        }
                }
                if (n >= active_month->len) break;
        }
}

void
render()
{
        window_clear(self_window, 0xDD000000, 0xDD000000);
        render_month();
}

struct tm *
today()
{
        time_t _t = time(0);
        struct tm *__t = localtime(&_t);
        mktime(__t);
        return __t;
}

void
task_dump(const char *filename)
{
        FILE *fp = fopen(filename, "w");
        if (!fp) {
                printf("Can not dump tasks: ");
                perror(filename);
                return;
        }

        for_da_each(elem, tasks)
        {
/*           */ #define T(x, ...)                   \
        /*           */ if (!strcmp(elem->name, x)) \
                /*           */ continue;
                LIST_OF_TASKS
/*           */ #undef T
                fprintf(fp, "[%s]\n", elem->name);
                fprintf(fp, "date = %s\n", repr_timet(elem->date));
                if (elem->desc) fprintf(fp, "desc = \"%s\"\n", elem->desc);
                if (elem->bg) fprintf(fp, "bg = %x\n", elem->bg);
                if (elem->fg) fprintf(fp, "fg = %x\n", elem->fg);
                fprintf(fp, "\n");
        }
}

void
task_parse_table(toml_table_t *table)
{
        printf("Parsing table %s\n", table->key);
        Task t = { 0 };
        if (table->key) t.name = strdup(table->key);

        for (int i = 0; i < table->nkval; i++) {
                if (0) {
                } else if (!strcmp("date", table->kval[i]->key)) {
                        toml_unparsed_t val = table->kval[i]->val;
                        toml_timestamp_t ts;

                        if (toml_value_timestamp(val, &ts)) {
                                printf("[%s]: Can not parse RCF 3339 timestamp from `%s`\n", t.name, val);
                                continue;
                        }

                        struct tm *now = today();

                        // set unset fields
                        if (ts.second < 0) ts.second = 0;
                        if (ts.minute < 0) ts.minute = 0;
                        if (ts.hour < 0) ts.hour = 0;
                        if (ts.day < 0) ts.day = now->tm_mday;
                        if (ts.month < 0) ts.month = now->tm_mon + 1;
                        if (ts.year < 0) ts.year = now->tm_year + 1900;


                        printf("-- Timestamp --\n");
                        printf("Seconds            %d \n", ts.second);
                        printf("Minutes            %d \n", ts.minute);
                        printf("Hour               %d \n", ts.hour);
                        printf("Day of the month   %d \n", ts.day);
                        printf("Month              %d \n", ts.month);
                        printf("Year               %d \n", ts.year);

                        struct tm date = (struct tm) {
                                .tm_sec = ts.second,       /* Seconds          [0, 60] */
                                .tm_min = ts.minute,       /* Minutes          [0, 59] */
                                .tm_hour = ts.hour,        /* Hour             [0, 23] */
                                .tm_mday = ts.day,         /* Day of the month [1, 31] */
                                .tm_mon = ts.month - 1,    /* Month            [0, 11]  (January = 0) */
                                .tm_year = ts.year - 1900, /* Year minus 1900 */
                                .tm_isdst = -1,            /* Daylight savings flag */
                        };

                        if ((t.date = mktime(&date)) < 0) {
                                perror("mktime");
                                t.date = 0;
                                continue;
                        }

                } else if (!strcmp("desc", table->kval[i]->key)) {
                        printf("No yet implemented: `desc` parsing\n");
                } else if (!strcmp("fg", table->kval[i]->key)) {
                        printf("No yet implemented: `fg` parsing\n");
                } else if (!strcmp("bg", table->kval[i]->key)) {
                        printf("No yet implemented: `bg` parsing\n");
                } else {
                        printf("Unknown key %s; use one of "
                               "`date`, `desc`, `fg` or `bg`\n",
                               table->kval[i]->key);
                }
        }

        for (int i = 0; i < table->narr; i++) {
                printf("Unsupported array tasks: %s\n", table->arr[i]->key);
        }

        for (int i = 0; i < table->ntbl; i++) {
                if (table->key) {
                        printf("Unsupported nested tasks: %s\n", table->tbl[i]->key);
                }
                task_parse_table(table->tbl[i]);
        }

        if (table->key) {
                if (!t.name) {
                        printf("Invalid task! Don't forget the name ([name])\n");
                        return;
                }
                if (!t.date) {
                        printf("Invalid task! Don't forget the date (date = 2027-02-58)\n");
                        return;
                }
                printf("Appending task %s\n", t.name);
                da_append(&tasks, t);
        }
}

void
add_tasks(const char *filename)
{
/*   */ #define T(a, b, c, d, e) \
        da_append(&tasks, (Task) { a, b, c, d, e });
        LIST_OF_TASKS
/*   */ #undef T

        char errbuf[128] = { 0 };
        FILE *fp;

        fp = fopen(filename, "r");
        if (!fp) {
                printf("Can not load tasks: ");
                perror(filename);
                return;
        }

        toml_table_t *table = toml_parse_file(fp, errbuf, sizeof errbuf - 1);
        if (!table) {
                fprintf(stderr, "toml parser: %s\n", errbuf);
                return;
        }
        task_parse_table(table);
        toml_free(table);
}

int
main(int argc, char **argv)
{
        if (argc <= 1) {
                fprintf(stderr, "Invalid arguments! Usage: %s tasks.toml\n", argv[0]);
                exit(0);
        }

        add_tasks(argv[1]);
        active_month = get_current_month();
        cursor = today()->tm_mday - 1;

        mainloop();

        task_dump(argv[1]);
        month_close(active_month);
        return 0;
}
