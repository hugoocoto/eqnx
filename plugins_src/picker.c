#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
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
} entry_arr = { 0 };
static int selected = 0;
static int rows = 0;

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
        window_px_to_coords(0, h, 0, &rows);
        selected = 0;
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        if (sym == 'j') selected = (selected + 1) % (entry_arr.size - 1);
        if (sym == 'k') selected = ((entry_arr.size - 1) + selected - 1) % (entry_arr.size - 1);
        if (sym == XKB_KEY_Return) plug_replace_img(self_plugin, build_path(PATH, entry_arr.data[selected].name));
        ask_for_redraw();
}

void
render()
{
        if (rows == 0) return;
        int page = selected / (rows - 1);
        int max = (page + 1) * (rows - 1);
        int width = 20;
        if (max > entry_arr.size - 1) max = entry_arr.size - 1;

        fb_clear(BACKGROUND);
        window_printf(self_window, 0, 0, YELLOW, BACKGROUND,
                      "Plugin picker [%d/%d]", selected + 1, entry_arr.size - 1);
        for (int i = page * (rows - 1); i < max; i++) {
                if (i == selected) {
                        window_printf(self_window, 0, i - page * (rows - 1) + 1, BACKGROUND, FOREGROUND,
                                      "  %-*.*s",
                                      width, width,
                                      entry_arr.data[i].name);

                        continue;
                }
                window_printf(self_window, 0, i - page * (rows - 1) + 1, FOREGROUND, BACKGROUND,
                              "  %-*.*s",
                              width, width,
                              entry_arr.data[i].name);
        }
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
        mainloop();
        return 0;
}
