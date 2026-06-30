#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

size_t *size;
char **buf;
void (**callback)(void);

void
notify()
{
        printf("Notify!\n");
        ask_for_redraw();
}

void
resize(int x, int y, int w, int h)
{
        ask_for_redraw();
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
        for (i = 0; i < *size && (*buf)[i]; i++) {
                row = i % self_window->w;
                col = i / self_window->w;
                window_set(self_window, row, col, (*buf)[i], fg, bg);
        }
        row = i % self_window->w;
        col = i / self_window->w;
        window_set(self_window, row, col, (*buf)[i] ?: ' ', fg, bg);
}

int
main(int argc, char **argv)
{
        if (argc != 4) {
                printf("Invalid args: %d\n"
                       "Usage: %s "
                       "<char* buffer reference symbol> "
                       "<int size reference symbol> "
                       "<void(*)(void) notify callback reference symbol>\n",
                       argc, argv[0]);
                exit(0);
        }

        symbol_register(argv[1], (void **) &buf, sizeof(char *));
        symbol_register(argv[2], (void **) &size, sizeof(size_t));
        symbol_register(argv[3], (void **) &callback, sizeof(void (*)(void)));
        *size = 16;
        *buf = calloc(1, *size);
        *callback = notify;

        assert(buf);
        assert(size);
        assert(callback);
        assert(*buf);
        printf("----------\n");
        printf("buf: %p, size: %zu, callback: %p\n", *buf, *size, *callback);
        printf("----------\n");

        mainloop();
        return 0;
}
