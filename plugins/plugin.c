#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

#define GRAY 0xFF666666
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

static Window *window;
static Plugin *plug;

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
        printf("Sym: %d\n", sym);
        if (sym == ' ') {
                printf("Replacing image with %s\n", "./plugins/color_red.so");
                plug_replace_img(plug, "./plugins/color_red.so");
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
        window_setall(window, '0', RED, BLACK);
        draw_window(window);
}

int
main(int argc, char **argv)
{
        window = request_window();
        plug = request_plug_info();

        printf("Plugin %s start\n", argv[0]);
        mainloop();
        printf("Plugin %s end\n", argv[0]);
        return 0;
}
