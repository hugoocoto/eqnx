#include <assert.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"

Plugin *child;
int last_pressed_char = ' ';

void
resize(int w, int h)
{
        printf("New size: %d %d\n", w, h);
}

void
kp_event(int sym, int mods)
{
        last_pressed_char = sym;
        plug_send_kp_event(child, sym, mods);
        ask_for_redraw();
}

Window *my_window;

void
render()
{
        assert(my_window);
        printf("PLUGIN: render\n");
        if (last_pressed_char) window_setall(my_window, last_pressed_char);
        draw_window(my_window);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        printf("(Plugin: %s) Hello!\n", argv[0]);
        my_window = request_window();
        printf("Plugin window: %p\n", my_window);
        mainloop();
        printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
