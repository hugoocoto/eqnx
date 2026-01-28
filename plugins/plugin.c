#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

#define GRAY 0xFF666666
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

static Window *my_window;
static Window *split_left;
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
        my_window = request_window();
        int sw = my_window->w / 2;

        split_left = window_cut(my_window, 0, 0, sw, my_window->h);
        split_right = window_cut(my_window, sw, 0, my_window->w - sw, my_window->h);
        assert(split_left);
        assert(split_right);
        plug_left = plug_run("./plugins/color_blue.so", split_left);
        plug_right = plug_run("./plugins/color_red.so", split_right);
        assert(plug_right);
        assert(plug_left);

        ask_for_redraw();
        mainloop();
        return 0;
}
