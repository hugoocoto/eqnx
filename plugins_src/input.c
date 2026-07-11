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

size_t pointer = 0;

void
resize(int px, int py, int pw, int ph)
{
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        // sanity checks for debug reasons
        assert(buf);
        assert(size);
        assert(callback);
        assert(*buf);

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
                        if (*callback) (*callback)();
                } else {
                        printf("Overflow!\n");
                }
        }
}

void
pointer_event(Pointer_Event e)
{
}

void
render()
{
        uint32_t fg = BACKGROUND;
        uint32_t bg = YELLOW;
        window_clear(self_window, bg, bg);
        window_puts(self_window, 0, 0, fg, bg, "This window sends input via IPC");
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

        symbol_register(argv[1], (void **) &buf, sizeof(char *));
        symbol_register(argv[2], (void **) &size, sizeof(size_t));
        symbol_register(argv[3], (void **) &callback, sizeof(void (*)(void)));

        assert(buf);
        assert(size);
        assert(callback);
        printf("----------\n");
        printf("buf: %p, size: %zu, callback: %p\n", *buf, *size, *callback);
        printf("----------\n");

        mainloop();
        return 0;
}
