#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"
#include "da.h"

#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define YELLOW 0xFFFFFF00
#define BLUE 0xFF0000FF
#define MAGENTA 0xFFFF00FF
#define CYAN 0xFF00FFFF
#define WHITE 0xFFFFFFFF

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
        printf("Picker) Getting real window size\n");
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
        printf("plugin_render rows=%d\n", rows);
        if (rows == 0) return;
        int page = selected / rows;
        int max = (page + 1) * rows;
        if (max > entry_arr.size - 1) max = entry_arr.size - 1;

        printf("plugin render clearing window\n");
        draw_clear_window(self_window);
        printf("plugin render itering from %d to %d\n", page * rows, max);
        for (int i = page * rows; i < max; i++) {
                printf("%d\n", i);
                if (i == selected) {
                        window_puts(self_window, 0, i - page * rows, entry_arr.data[i].name, BLACK, WHITE);
                        continue;
                }
                window_puts(self_window, 0, i - page * rows, entry_arr.data[i].name, WHITE, BLACK);
        }
}

void
add_entries(DIR *dir)
{
        struct dirent *entry;
        printf("Adding entries\n");
        while ((entry = readdir(dir))) {
                if (!strendswith(entry->d_name, ".so")) continue;
                da_append(&entry_arr, (Entry) {
                                      .name = strdup(entry->d_name),
                                      .type = 0,
                                      });
        }
        printf("entries: %d\n", entry_arr.size);
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
