#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/plug_api.h"

#define GRAY 0xFF666666
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

Window *my_window;

void
resize(int x, int y, int w, int h)
{
        printf("New size: %d %d\n", w, h);
        draw_clear_window(my_window);
        char str[1024];
        snprintf(str, sizeof str - 1, "New size: %d, %d", w, h);
        window_puts(my_window, w / 2 - strlen(str), h / 2, str, RED, WHITE);
        ask_for_redraw();
}

void
kp_event()
{
        ask_for_redraw();
}

void
render()
{
        draw_window(my_window);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        my_window = request_window();
        ask_for_redraw();
        mainloop();
        return 0;
}
