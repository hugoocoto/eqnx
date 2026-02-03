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
                uint32_t c = strtoull(argv[1], NULL, 10);
                color = c < (int) COLORS_LEN ? COLORS[c] : c;
        }
        printf("Color color: %s -> 0x%X\n", argv[1], color);
        mainloop();
        return 0;
}
