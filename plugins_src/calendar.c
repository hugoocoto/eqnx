#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>
#include <time.h>

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

/* Month [0, 11] (January = 0) */
Month *
month_open(int month, int year)
{
        printf("month open :: %d %d\n", month, year);
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
                                fg = FOREGROUND;
                                bg = GREEN;
                                asc_fg = GREEN;
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
        window_clear(self_window, BACKGROUND, BACKGROUND);
        render_month();
}

#define LIST_OF_TASKS \
        T("now", 0, time(0), BACKGROUND, YELLOW)

void
add_tasks()
{
#define T(a, b, c, d, e) \
        da_append(&tasks, (Task) { a, b, c, d, e });
        LIST_OF_TASKS
#undef T
}

int
main(int argc, char **argv)
{
        add_tasks();
        active_month = get_current_month();
        mainloop();
        month_close(active_month);
        return 0;
}
