#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

int last_pressed_char = ' ';
Window *my_window;

void
resize(int w, int h)
{
        printf("New size: %d %d\n", w, h);
}

void
kp_event(int sym, int mods)
{
        if (sym == ' ') {
                assert(fb_capture("screen_capture.png"));
                return;
        }
        last_pressed_char = sym;
        ask_for_redraw();
}

void
render()
{
        assert(my_window);
        if (last_pressed_char) window_setall(my_window, last_pressed_char, BLACK, GREEN);
        draw_window(my_window);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        // printf("(Plugin: %s) Hello!\n", argv[0]);
        my_window = request_window();
        mainloop();
        // printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
