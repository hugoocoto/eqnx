#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

static size_t *size;
static char **buf;
static size_t pointer = 0;
static void (*callback)();

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
        switch (sym) {
        case XKB_KEY_Left:
                if (pointer > 0) --pointer;
                break;
        case XKB_KEY_Right:
                if (pointer < *size && (*buf)[pointer]) ++pointer;
                break;
        case ' ' ... 127:
                if (pointer < *size) {
                        (*buf)[pointer] = sym;
                        ++pointer;
                        if (callback) callback();
                } else {
                        printf("Overflow!\n");
                }
        }
        ask_for_redraw();
}

void
pointer_event(Pointer_Event e)
{
}

void
render()
{
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

        buf = symbol_get(argv[1]);
        size = symbol_get(argv[2]);
        callback = symbol_get(argv[3]);

        printf("Buffer: %p (size %p) callback %p\n", buf, size, callback);
        if (size) printf("--> size %zu\n", *size);

        if (buf == 0 || size == 0 || callback == 0) exit(1);

        mainloop();
        return 0;
}
