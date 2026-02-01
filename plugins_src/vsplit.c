#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window;
Window *split_left;
static Window *split_right;
static Plugin *plug_left;
static Plugin *plug_right;

void
resize(int x, int y, int w, int h)
{
        assert(x == 0);
        assert(y == 0);
        int sw = w / 2;
        plug_send_resize_event(plug_left, x, y, sw, h);
        plug_send_resize_event(plug_right, x + sw, y, w - sw, h);
}

void
kp_event(int sym, int mods)
{
        plug_send_kp_event(plug_left, sym, mods);
        plug_send_kp_event(plug_right, sym, mods);
}

void
pointer_event(Pointer_Event e)
{
        plug_send_mouse_event(plug_left, e);
        plug_send_mouse_event(plug_right, e);
}

void
render()
{
        plug_left->render();
        plug_right->render();
}

int
main(int argc, char **argv)
{
        int sw = self_window->w / 2;

        split_left = window_cut(self_window, 0, 0, sw, self_window->h);
        split_right = window_cut(self_window, sw, 0, self_window->w - sw, self_window->h);
        assert(split_left);
        assert(split_right);
        plug_left = plug_run("./plugins/picker.so", split_left);
        plug_right = plug_run("./plugins/picker.so", split_right);
        assert(plug_right);
        assert(plug_left);

        ask_for_redraw();
        mainloop();
        return 0;
}
