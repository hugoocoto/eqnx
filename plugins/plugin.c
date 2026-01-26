#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

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
        last_pressed_char = sym;
        ask_for_redraw();
}

void
render()
{
        assert(my_window);
        if (last_pressed_char) window_setall(my_window, last_pressed_char);
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
