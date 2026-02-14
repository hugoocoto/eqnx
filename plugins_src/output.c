#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

static size_t size = 100;
static char *buf;

void
notify()
{
        printf("Notify!\n");
}

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
}

void
pointer_event(Pointer_Event e)
{
}

void
render()
{
        int row, col;
        uint32_t fg = FOREGROUND;
        uint32_t bg = BACKGROUND;
        window_clear(self_window, BACKGROUND, BACKGROUND);
        size_t i;
        for (i = 0; i < size && buf[i]; i++) {
                row = i % self_window->w;
                col = i / self_window->w;
                window_set(self_window, row, col, buf[i], fg, bg);
        }
        row = i % self_window->w;
        col = i / self_window->w;
        window_set(self_window, row, col, buf[i] ?: ' ', fg, bg);
}

int
main(int argc, char **argv)
{
        if (argc != 4) {
                printf("Invalid args: %d\n"
                       "Usage: %s <char* buffer reference symbol> "
                       "<int size reference symbol> "
                       "<void(*)(void) notify callback reference symbol>\n",
                       argc, argv[0]);
                exit(0);
        }

        buf = calloc(1, size);

        symbol_add(argv[1], &buf);
        printf("Adding symbol %s (%p)\n", argv[1], &buf);
        symbol_add(argv[2], &size);
        printf("Adding symbol %s (%p)\n", argv[2], &size);
        symbol_add(argv[3], &notify);
        printf("Adding symbol %s (%p)\n", argv[3], notify);

        mainloop();
        return 0;
}
