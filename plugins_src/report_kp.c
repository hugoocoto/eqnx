#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

void
resize(int x, int y, int w, int h)
{
        // (How to) get size in chars
        // int cx, cy, cw, ch;
        // window_px_to_coords(x, y, &cx, &cy);
        // window_px_to_coords(w, h, &cw, &ch);
        ask_for_redraw();
}

static int last_sym = 0;

void
kp_event(int sym, int mods)
{
        last_sym = sym;
        ask_for_redraw();
}

void
pointer_event(Pointer_Event e)
{
}

void
render()
{
        window_clear(self_window, BACKGROUND, BACKGROUND);
        window_set(self_window, 0, 0, last_sym, WHITE, RED);
        printf("sym: %d\n", last_sym);
}

int
main(int argc, char **argv)
{
        mainloop();
        return 0;
}
