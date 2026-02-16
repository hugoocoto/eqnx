#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../thirdparty/toml-c.h"
#undef calloc
#undef strdup
#undef strndup

#include "../src/plug_api.h"
#include "da.h"

#include "theme.h"
#undef BACKGROUND
#define BACKGROUND 0xdd000000 // semi transparent background

#define SEP 1
#define INITIAL_Y SEP
#define INITIAL_X 0

typedef struct Month Month;

Window *self_window = NULL;
Plugin *self_plugin = NULL;
Month *active_month = NULL;

static int month_cell_height;
static int month_cell_width;
static int cursor = 0;
static int day_selection = 0;
const int entries_per_task = 4;
static enum {
        Render_Month,
        Render_Day,
        Render_Form,
} render_state = Render_Month;

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

Task *get_task_between(time_t t0, time_t t1, int index);
int count_task_between(time_t t0, time_t t1, int index);

struct Task_DA {
        int capacity;
        int size;
        Task *data;
} tasks = { 0 };

static struct {
        Task *t;
        const char *field;
        struct {
                int capacity;
                int size;
                char *data;
        } input;
        enum {
                FORM_CANCEL,
                FORM_OK,
        } status;
} form_info = { 0 };

typedef struct Button {
        int x, y, w, h;
        void (*action)(int x, int y);
} Button;

struct {
        int capacity;
        int size;
        Button *data;
} buttons;

/* List of constant predefined tasks */
#define LIST_OF_TASKS \
        T("now", 0, time(0), BACKGROUND, YELLOW)

struct tm *
today()
{
        time_t _t = time(0);
        struct tm *__t = localtime(&_t);
        mktime(__t);
        return __t;
}

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

const char *const numbers[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
        "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25",
        "26", "27", "28", "29", "30", "31"
};

const char *
repr_tm(struct tm *tm, const char *fmt)
{
        static char buf[128];
        memset(buf, 0, sizeof buf);
        strftime(buf, sizeof buf - 1, fmt, tm);
        return buf;
}

const char *
repr_day(int day)
{
        return repr_tm(&active_month->days[day].tm, "%a %d %b");
}

const char *
repr_today()
{
        return repr_tm(today(), "%a %d %b");
}

const char *
repr_month(Month *m)
{
        return repr_tm(&m->days->tm, "%b");
}

const char *
repr_year(Month *m)
{
        return repr_tm(&m->days->tm, "%Y");
}

const char *
repr_timer(time_t timer)
{
        return repr_tm(localtime(&timer), "%F %T");
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
        struct tm tm = m->days[0].tm;
        int mon = (tm.tm_mon + 1) % 12;
        int year = tm.tm_year + (tm.tm_mon == 11);
        return month_open(mon, year + 1900);
}

Month *
get_prev_month(Month *m)
{
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
close_form()
{
        if (form_info.status == FORM_OK) {
                if (!strcmp(form_info.field, "name") && form_info.input.data[0]) {
                        form_info.t->name = strdup(form_info.input.data);
                }
                if (!strcmp(form_info.field, "desc")) {
                        if (form_info.input.data[0] == 0) {
                                form_info.t->desc = 0;
                        } else {
                                form_info.t->desc = strdup(form_info.input.data);
                        }
                }
        }
        memset(&form_info, 0, sizeof form_info);

        render_state = Render_Day;
        ask_for_redraw();
}

void
open_form(Task *t, const char *field)
{
        render_state = Render_Form;
        form_info.input.size = 0;

        if (!strcmp(field, "name") && t->name) {
                for (char *c = t->name; *c; ++c) {
                        da_append(&form_info.input, *c);
                }
        } else if (!strcmp(field, "desc") && t->desc) {
                for (char *c = t->desc; *c; ++c) {
                        da_append(&form_info.input, *c);
                }
        }
        da_append(&form_info.input, 0);
        form_info.t = t;
        form_info.field = field;
        ask_for_redraw();
}

void
open_day_view(int day)
{
        render_state = Render_Day;
        cursor = day - 1;
        day_selection = 0;
        ask_for_redraw();
}

void
close_day_view()
{
        render_state = Render_Month;
        ask_for_redraw();
}

void
drop_all_buttons()
{
        buttons.size = 0;
}

void
button_go_month(int x, int y)
{
        close_day_view();
}

void
button_go_day(int x, int y)
{
        close_form();
}

void
button_prev_day(int x, int y)
{
        if (cursor-- <= 0) {
                active_month = get_prev_month(active_month);
                cursor = active_month->len - 1;
        }
        ask_for_redraw();
}

void
button_next_day(int x, int y)
{
        if (++cursor >= active_month->len) {
                active_month = get_next_month(active_month);
                cursor = 0;
        }
        ask_for_redraw();
}

void
button_prev_week(int x, int y)
{
        // guarrada historica
        for (int i = 0; i < 7; i++) {
                button_prev_day(0, 0);
        }
        ask_for_redraw();
}

void
button_next_week(int x, int y)
{
        for (int i = 0; i < 7; i++) {
                button_next_day(0, 0);
        }
        ask_for_redraw();
}


void
button_prev_year(int x, int y)
{
        // guarrada historica
        for (int i = 0; i < 12; i++) {
                active_month = get_prev_month(active_month);
        }
        cursor = 0;
        ask_for_redraw();
}

void
button_next_year(int x, int y)
{
        for (int i = 0; i < 12; i++) {
                active_month = get_next_month(active_month);
        }
        cursor = 0;
        ask_for_redraw();
}

void
button_prev_month(int x, int y)
{
        active_month = get_prev_month(active_month);
        cursor = 0;
        ask_for_redraw();
}

void
button_next_month(int x, int y)
{
        active_month = get_next_month(active_month);
        cursor = 0;
        ask_for_redraw();
}

void
button_today(int x, int y)
{
        active_month = get_current_month();
        cursor = today()->tm_mday - 1;
        ask_for_redraw();
}

void
button_edit_name(int x, int y)
{
        int task_n = (y - 1) / (entries_per_task + 1);
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        Task *t = get_task_between(t0, t1, task_n);
        assert(t);
        open_form(t, "name");
}

void
button_edit_desc(int x, int y)
{
        int task_n = (y - 1) / (entries_per_task + 1);
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        Task *t = get_task_between(t0, t1, task_n);
        assert(t);
        open_form(t, "desc");
}

void
button_edit_date(int x, int y)
{
        int task_n = (y - 1) / (entries_per_task + 1);
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        Task *t = get_task_between(t0, t1, task_n);
        assert(t);
        open_form(t, "date");
}

void
button_delete_task(int x, int y)
{
        int task_n = (y - 1) / (entries_per_task + 1);
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        Task *t = get_task_between(t0, t1, task_n);
        assert(t);
        int task_index = da_index(tasks, t);
        assert(task_index >= 0 && task_index < tasks.size);
        da_remove(&tasks, task_index);
        ask_for_redraw();
}

void
button_form_back(int x, int y)
{
        close_form();
}

void
button_form_ok(int x, int y)
{
        form_info.status = FORM_OK;
        close_form();
}

void
button_day_selection_down(int x, int y)
{
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        int entries = count_task_between(t0, t1, 0) * entries_per_task;
        if (entries == 0) return;
        day_selection = (day_selection + 1) % entries;
        ask_for_redraw();
}

void
button_day_selection_up(int x, int y)
{
        time_t t0 = active_month->days[cursor].time;
        time_t t1 = active_month->days[cursor + 1].time;
        int entries = count_task_between(t0, t1, 0) * entries_per_task;
        if (entries == 0) return;
        day_selection = (day_selection - 1 + entries) % entries;
        ask_for_redraw();
}

void
button_new_task(int x, int y)
{
        time_t t0 = active_month->days[cursor].time;
        Task t = (Task) {
                .date = t0,
        };
        da_append(&tasks, t);
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        if (render_state == Render_Form) {
                if (sym == XKB_KEY_BackSpace && form_info.input.size > 1) {
                        form_info.input.size -= 2;
                        da_append(&form_info.input, 0);
                } else if (sym == XKB_KEY_Return) {
                        form_info.status = FORM_OK;
                        close_form();
                } else if (' ' <= sym && sym <= 127 && isprint(sym)) {
                        form_info.input.size -= 1;
                        da_append(&form_info.input, sym);
                        da_append(&form_info.input, 0);
                } else if (sym == XKB_KEY_Escape) {
                        close_form();
                }
                ask_for_redraw();
                return;
        }

        if (sym == 'n' && mods | Mod_Control) {
                button_next_month(0, 0);
        }

        else if (sym == 'p' && mods | Mod_Control) {
                button_prev_month(0, 0);
        }

        else if (sym == XKB_KEY_Return) {
                if (render_state == Render_Month)
                        open_day_view(cursor + 1);
                else if (render_state == Render_Day) {
                        if (day_selection % entries_per_task == 0) {
                                button_edit_name(0, day_selection + day_selection / entries_per_task + 1);
                        } else if (day_selection % entries_per_task == 1) {
                                button_edit_desc(0, day_selection + day_selection / entries_per_task + 1);
                        } else if (day_selection % entries_per_task == 2) {
                                button_edit_date(0, day_selection + day_selection / entries_per_task + 1);
                        } else if (day_selection % entries_per_task == 3) {
                                button_delete_task(0, day_selection + day_selection / entries_per_task + 1);
                        }
                }
        }

        else if (sym == XKB_KEY_Escape) {
                if (render_state == Render_Day) {
                        close_day_view();
                }
        }

        else if (sym == 'h' && mods == Mod_None) {
                if (render_state == Render_Month) {
                        if (cursor > 0) --cursor;
                        ask_for_redraw();
                } else if (render_state == Render_Day) {
                        button_prev_day(0, 0);
                }

        } else if (sym == 'j' && mods == Mod_None) {
                if (render_state == Render_Month) {
                        ask_for_redraw();
                        if ((cursor += 7) >= active_month->len) cursor = active_month->len - 1;
                } else if (render_state == Render_Day) {
                        button_day_selection_down(0, 0);
                }

        } else if (sym == 'k' && mods == Mod_None) {
                if (render_state == Render_Month) {
                        if ((cursor -= 7) < 0) cursor = 0;
                        ask_for_redraw();
                } else if (render_state == Render_Day) {
                        button_day_selection_up(0, 0);
                }

        } else if (sym == 'l' && mods == Mod_None) {
                if (render_state == Render_Month) {
                        if (cursor < active_month->len - 1) ++cursor;
                        ask_for_redraw();
                } else if (render_state == Render_Day) {
                        button_next_day(0, 0);
                }
        }

        else if (sym == 'c' && mods == Mod_None) {
                if (render_state == Render_Day) button_new_task(0, 0);
        }
}

int
pointer_get_month_day(int x, int y, int *h)
{
        if (y < INITIAL_Y || x < INITIAL_X || x >= month_cell_width * 7) {
                return 0;
        }

        int offset = active_month->days[0].tm.tm_wday;
        int row = (y - INITIAL_Y) / month_cell_height;
        int col = (x - INITIAL_X) / month_cell_width;
        int is_sep = (x - INITIAL_X) % month_cell_width < SEP;
        if (h) *h = (y - INITIAL_Y) % month_cell_height;
        if (row == 0 && col < offset) return 0;

        int day = row * 7 + col - offset + 1;
        if (day <= 0 || day > active_month->len) return 0;

        return day;
}

void
pointer_event(Pointer_Event e)
{
        static int last_day_pressed = 0;
        static int row = 0;

        if (e.type == Pointer_Press) {
                for_da_each(but, buttons)
                {
                        if (but->x <= e.x && but->x > e.x - but->w &&
                            but->y <= e.y && but->y > e.y - but->h) {
                                but->action(e.x, e.y);
                        }
                }
        }

        if (render_state == Render_Month && e.type == Pointer_Press) {
                last_day_pressed = pointer_get_month_day(e.x, e.y, &row);
        }

        if (render_state == Render_Month && e.type == Pointer_Release) {
                int day = pointer_get_month_day(e.x, e.y, 0);
                if (last_day_pressed > 0 && day == last_day_pressed) {
                        open_day_view(day);
                        ask_for_redraw();
                }
                if (last_day_pressed > 0 && day != last_day_pressed) {
                        time_t t0 = active_month->days[last_day_pressed - 1].time;
                        time_t t1 = active_month->days[last_day_pressed].time;
                        Task *t = get_task_between(t0, t1, row - 1);
                        if (!t) return;
                        struct tm *tm = localtime(&t->date);
                        tm->tm_mday = day;
                        t->date = mktime(tm);
                }
        }

        if (render_state == Render_Month && e.type == Pointer_Move) {
                int day = pointer_get_month_day(e.x, e.y, 0);
                cursor = day > 0 ? day - 1 : cursor;
                ask_for_redraw();
        }
}

int
count_task_between(time_t t0, time_t t1, int index)
{
        int i = 0;
        int count = 0;
        for (; i < tasks.size; i++) {
                if (tasks.data[i].date >= t0 && tasks.data[i].date < t1) {
                        ++count;
                }
        }
        return count - index;
}

Task *
get_task_between(time_t t0, time_t t1, int index)
{
        int i = 0;
        for (; i < tasks.size; i++) {
                if (tasks.data[i].date >= t0 && tasks.data[i].date < t1) {
                        if (index == 0) {
                                return tasks.data + i;
                        }
                        --index;
                }
        }
        return NULL;
}

int
set_button(Window *window, int x, int y, uint32_t fg, uint32_t bg, void (*action)(int, int), char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        int len = window_vprintf(window, x, y, fg, bg, fmt, ap);
        va_end(ap);
        da_append(&buttons, (Button) { .x = x, .y = y, .w = len, .h = 1, .action = action });
        return len;
}

void
open_hugo_web(int x, int y)
{
        int pid;
        pid = fork();
        if (pid == 0) {
                execlp("xdg-open", "xdg-open", "https://hugocoto.com", NULL);
                perror("xdg-open");
                exit(0);
        }
}

void
render_month()
{
        drop_all_buttons();
        if (month_cell_height < 2 || month_cell_width < 4) {
                window_clear(self_window, RED, RED);
                window_puts(self_window, 0, 0, BLACK, RED, "Screen too small");
                return;
        }

        int offset = active_month->days[0].tm.tm_wday;
        uint32_t fg, bg, bg2, fg2;
        int n = 0, i, j, k;
        int off = 0;

        // stack top bar
        off += set_button(self_window, 0, 0, BLUE, BACKGROUND, button_prev_month, " < ");
        off += window_printf(self_window, off, 0, FOREGROUND, BACKGROUND, "%s", repr_month(active_month));
        off += set_button(self_window, off, 0, BLUE, BACKGROUND, button_next_month, " > ");
        off += set_button(self_window, off, 0, BLUE, BACKGROUND, button_prev_year, " < ");
        off += window_printf(self_window, off, 0, FOREGROUND, BACKGROUND, "%s", repr_year(active_month));
        off += set_button(self_window, off, 0, BLUE, BACKGROUND, button_next_year, " > ");
        off += set_button(self_window, off, 0, YELLOW, BACKGROUND, button_today, " %s ", repr_today());
        off += set_button(self_window, off, 0, BLUE, BACKGROUND, open_hugo_web, "   Hugo's Calendar  ");

        assert(offset >= 0 && offset <= 6 * month_cell_width);
        for (j = 0;; j += month_cell_height) {
                for (i = offset; i < 7 && n < active_month->len; i++, n++, offset = 0) {
                        assert(n >= 0 && n <= 30);

                        fg = cursor != n ? FOREGROUND : BLACK;
                        bg = cursor != n ? GRAY : WHITE;
                        fg2 = cursor != n ? BLUE : WHITE;
                        bg2 = cursor != n ? BACKGROUND : BACKGROUND;

                        window_printf(self_window, i * month_cell_width + SEP, j + INITIAL_Y, fg2, bg2, "%*.*s ",
                                      month_cell_width - 1 - SEP, month_cell_width - 1 - SEP, numbers[n]);
                        for (k = 1; k < month_cell_height; k++) {
                                time_t t0 = active_month->days[n].time;
                                time_t t1 = active_month->days[n + 1].time;
                                if (k + 1 == month_cell_height) {
                                        int count = count_task_between(t0, t1, k - 1);
                                        if (count > 1) {
                                                window_printf(self_window, i * month_cell_width + SEP, j + k + INITIAL_Y, fg, bg, "+%-*d",
                                                              month_cell_width - 1 - SEP, count);
                                                continue;
                                        }
                                }
                                Task *t;
                                if ((t = get_task_between(t0, t1, k - 1))) {
                                        window_printf(self_window, i * month_cell_width + SEP, j + k + INITIAL_Y, t->fg ?: fg, t->bg ?: bg, "%-*.*s",
                                                      month_cell_width - SEP, month_cell_width - SEP, t->name);
                                } else {
                                        window_printf(self_window, i * month_cell_width + SEP, j + k + INITIAL_Y, fg, bg, "%-*.*s",
                                                      month_cell_width - SEP, month_cell_width - SEP, "");
                                }
                        }
                }
                if (n >= active_month->len) break;
        }
}

void
render_day()
{
        drop_all_buttons();
        if (month_cell_height < 2 || month_cell_width < 4) {
                window_clear(self_window, RED, RED);
                window_puts(self_window, 0, 0, BLACK, RED, "Screen too small");
                return;
        }

        int off = 0;
        int yoff = 0;

        // stack top bar
        off += set_button(self_window, off, yoff, RED, BACKGROUND, button_go_month, " back ");
        off += set_button(self_window, off, yoff, BLUE, BACKGROUND, button_prev_day, " < ");
        off += window_printf(self_window, off, yoff, FOREGROUND, BACKGROUND, "%s", repr_day(cursor));
        off += set_button(self_window, off, yoff, BLUE, BACKGROUND, button_next_day, " > ");
        off += set_button(self_window, off, yoff, GREEN, BACKGROUND, button_new_task, " new  ");
        ++yoff;

        int entry_n = 0;
        for (int i = 0;; ++i) {
                time_t t0 = active_month->days[cursor].time;
                time_t t1 = active_month->days[cursor + 1].time;
                Task *t = get_task_between(t0, t1, i);
                if (t == NULL) break;

                uint32_t fg, bg;

                fg = day_selection == entry_n ? BACKGROUND : FOREGROUND;
                bg = day_selection == entry_n ? FOREGROUND : BACKGROUND;

                set_button(self_window, 0, yoff++, fg, bg, button_edit_name, "Name: %s", t->name);
                ++entry_n;

                fg = day_selection == entry_n ? BACKGROUND : FOREGROUND;
                bg = day_selection == entry_n ? FOREGROUND : BACKGROUND;

                set_button(self_window, 0, yoff++, fg, bg, button_edit_desc, "Desc: %s", t->desc);
                ++entry_n;

                fg = day_selection == entry_n ? BACKGROUND : FOREGROUND;
                bg = day_selection == entry_n ? FOREGROUND : BACKGROUND;

                set_button(self_window, 0, yoff++, fg, bg, button_edit_date, "Date: %s", repr_timer(t->date));
                ++entry_n;

                fg = day_selection == entry_n ? BACKGROUND : RED;
                bg = day_selection == entry_n ? RED : BACKGROUND;

                set_button(self_window, 0, yoff++, fg, bg, button_delete_task, "Delete");
                ++entry_n;
                ++yoff;
        }
}

void
render_form()
{
        drop_all_buttons();
        int off = 0;
        int yoff = 0;
        off += set_button(self_window, off, yoff, RED, BACKGROUND, button_go_day, " back ");
        off += window_printf(self_window, off, yoff, FOREGROUND, BACKGROUND,
                             "task %s", form_info.t->name);
        off += set_button(self_window, off, yoff, GREEN, BACKGROUND, button_form_ok, " ok ");
        ++yoff;
        ++yoff;

        off = 0;
        off += window_printf(self_window, off, yoff, FOREGROUND, BACKGROUND,
                             "%s = %s_", form_info.field, form_info.input.data);
}

void
render()
{
        window_clear(self_window, BACKGROUND, BACKGROUND);
        switch (render_state) {
        case Render_Month: render_month(); break;
        case Render_Day: render_day(); break;
        case Render_Form: render_form(); break;
        }
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
                if (elem->name == 0) {
                        printf("Task has no name! Ignoring it\n");
                        continue;
                }

/*           */ #define T(x, ...)                   \
        /*           */ if (!strcmp(elem->name, x)) \
                /*          */ continue;
                LIST_OF_TASKS
/*           */ #undef T
                fprintf(fp, "[%s]\n", elem->name);
                fprintf(fp, "date = %s\n", repr_timer(elem->date));
                if (elem->desc) fprintf(fp, "desc = \"%s\"\n", elem->desc);
                if (elem->bg) fprintf(fp, "bg = %x\n", elem->bg);
                if (elem->fg) fprintf(fp, "fg = %x\n", elem->fg);
                fprintf(fp, "\n");
        }
}

void
task_parse_table(toml_table_t *table)
{
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

                        // printf("-- Timestamp --\n");
                        // printf("Seconds            %d \n", ts.second);
                        // printf("Minutes            %d \n", ts.minute);
                        // printf("Hour               %d \n", ts.hour);
                        // printf("Day of the month   %d \n", ts.day);
                        // printf("Month              %d \n", ts.month);
                        // printf("Year               %d \n", ts.year);

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
                        char *ret;
                        int len;
                        toml_value_string(table->kval[i]->val, &ret, &len);
                        t.desc = strdup(ret);

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
