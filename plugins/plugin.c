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

extern int closedir(DIR *);
extern DIR *opendir(const char *);
extern struct dirent *readdir(DIR *);

typedef struct Entry {
        char *name;
        int type;
} Entry;

struct {
        int capacity;
        int size;
        Entry *data;
} entry_arr = { 0 };
int selected = 0;
int cols = -1;

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
        window_px_to_coords(w, 0, &cols, NULL);
        ask_for_redraw();
        selected = 0;
}

void
kp_event(int sym, int mods)
{
        if (sym == 'j') selected = (selected + 1) % (entry_arr.size - 1);
        if (sym == 'k') selected = ((entry_arr.size - 1) + selected - 1) % (entry_arr.size - 1);
        if (sym == XKB_KEY_Return) plug_replace_img(self_plugin, build_path(PATH, entry_arr.data[selected].name));
        if (sym == ' ') plug_replace_img(self_plugin, "./plugins/color_red.so");
        ask_for_redraw();
}

void
render()
{
        int max = entry_arr.size - selected - 1;
        if (max > cols) max = cols;
        printf("cols: %d - selected: %d\n", max, selected);

        if (max == 0) return;
        draw_clear_window(self_window);
        window_puts(self_window, 0, 0, entry_arr.data[selected].name, BLACK, WHITE);
        for (int i = 1; i < max; i++) {
                window_puts(self_window, 0, i, entry_arr.data[selected + i].name, WHITE, BLACK);
        }

        draw_window(self_window);
}

void
add_entries(DIR *dir)
{
        struct dirent *entry;
        while ((entry = readdir(dir))) {
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
        window_px_to_coords(self_window->w, 0, &cols, NULL);
        add_entries(dir);
        mainloop();
        return 0;
}
