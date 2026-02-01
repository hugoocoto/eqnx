#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"
#include "da.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

#define PATH "./plugins/"

typedef struct Entry {
        char *name;
        int type;
} Entry;

static struct {
        int capacity;
        int size;
        Entry *data;
} entry_arr = { 0 },
  match_entry_arr = { 0 };
static int selected = 0;
static int rows = 0;
static int cols = 0;
static char search_text[12] = "";

bool
strendswith(const char *str, const char *suf)
{
        if (!suf) return true;
        size_t suflen = strlen(suf);
        size_t len = strlen(str);
        return !strcmp(str + len - suflen, suf);
}

char *
build_path(char *path, char *name)
{
        static char buffer[1024];
        snprintf(buffer, sizeof buffer, "%s/%s", path, name);
        return buffer;
}

void
resize(int x, int y, int w, int h)
{
        window_px_to_coords(w, h, &cols, &rows);
        selected = 0;
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        if (sym == 'n' && mod_has_Control(mods) && match_entry_arr.size != 0) selected = (selected + 1) % (match_entry_arr.size);
        if (sym == 'p' && mod_has_Control(mods) && match_entry_arr.size != 0) selected = ((match_entry_arr.size) + selected - 1) % (match_entry_arr.size);
        if (sym == XKB_KEY_Return && match_entry_arr.size != 0) plug_replace_img(self_plugin, build_path(PATH, match_entry_arr.data[selected].name));
        if (sym == XKB_KEY_BackSpace) {
                if (search_text[0]) search_text[strlen(search_text) - 1] = 0;
        }
        if (isprint(sym) && sym < 128 && (mods == 0 || mods == Mod_Shift)) {
                strncat(search_text, (const char[]) { sym, 0 }, sizeof search_text - 1);
                selected = 0;
        }
        ask_for_redraw();
}

bool
match(char *s1, char *s2)
{
        int len1 = strlen(s1);
        int len2 = strlen(s2);

        int j = 0;
        for (int i = 0; i < len2; i++) {
                if (s2[i] == s1[j]) ++j;
                if (j == len1) return true;
        }
        return len1 == 0;
}

__attribute__((constructor)) void
test()
{
        puts("-- Test --");
        assert(match("", "hello"));
        assert(match("h", "hello"));
        assert(match("hl", "hello"));
        assert(match("hlo", "hello"));
        assert(!match("hlol", "hello"));
}

void
filter_matches()
{
        da_destroy(&match_entry_arr);
        memset(&match_entry_arr, 0, sizeof match_entry_arr);

        for_da_each(elem, entry_arr)
        {
                if (match(search_text, elem->name)) {
                        da_append(&match_entry_arr, *elem);
                }
        }
}

void
render()
{
        filter_matches();

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
        if (rows == 0) return;
        int R = rows - 2;
        int page = selected / (R);
        int max = (page + 1) * (R);
        int width = MIN(20, cols - 2);
        if (max > match_entry_arr.size) max = match_entry_arr.size;


        draw_clear_window(self_window, BACKGROUND, BACKGROUND);
        window_printf(self_window, 0, 0, YELLOW, BACKGROUND,
                      "Plugin picker [%d/%d]", selected + 1, match_entry_arr.size);

        for (int i = page * (R); i < max; i++) {
                char *c = strchr(match_entry_arr.data[i].name, '.');
                if (c) *c = 0;
                window_printf(self_window, 0, i - page * (R) + 1,
                              i == selected ? BACKGROUND : FOREGROUND,
                              i == selected ? FOREGROUND : BACKGROUND,
                              "  %-*.*s", width, width, match_entry_arr.data[i].name);
                if (c) *c = '.';
        }

        window_printf(self_window, 0, rows - 1, FOREGROUND, BACKGROUND,
                      "Search: %s", search_text);
}

void
add_entries(DIR *dir)
{
        struct dirent *entry;
        while ((entry = readdir(dir))) {
                if (!strendswith(entry->d_name, ".so")) continue;
                da_append(&entry_arr, (Entry) {
                                      .name = strdup(entry->d_name),
                                      .type = 0,
                                      });
        }
}

int
main(int argc, char **argv)
{
        DIR *dir = opendir(PATH);
        assert(dir);
        add_entries(dir);
        filter_matches();
        mainloop();
        return 0;
}
