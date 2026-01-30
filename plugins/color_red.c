#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

#define GRAY 0xFF666666
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

Window *self_window;

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
}

void
render()
{
        window_setall(self_window, ' ', RED, RED);
        draw_window(self_window);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        mainloop();
        return 0;
}
