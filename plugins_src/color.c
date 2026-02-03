#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window;
uint32_t color = RED;

void
resize()
{
        ask_for_redraw();
}

void
render()
{
        window_setall(self_window, ' ', color, color);
}

int
main(int argc, char **argv)
{
        if (argc == 2) {
                int c = atoi(argv[1]);
                color = c >= 0 && c < COLORS_LEN ? COLORS[c] : color;
        }
        printf("Color color: %s -> %x\n", argv[1], color);
        mainloop();
        return 0;
}
