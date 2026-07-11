#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

static Image *img = NULL;
static char *img_path = NULL;
static int load_failed = 0;

int
resize(int px, int py, int pw, int ph)
{
        ask_for_redraw();
        return 0;
}

int
kp_event(int sym, int mods)
{
        return 0;
}

int
pointer_event(Pointer_Event e)
{
        return 0;
}

int
render()
{
        if (load_failed) {
                window_clear(self_window, RED, RED);
                window_printf(self_window, 0, 0, BLACK, RED,
                              "Image <%s> not found", img_path);
        } else if (img) {
                window_clear(self_window, BLUE, BLUE);
                printf("draw_image_scaled %d,%d to %d,%d\n", 0, 0, self_window->w, self_window->h);
                draw_image_scaled(img, 0, 0, self_window->w, self_window->h);
        }
        return 0;
}

int
main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage: %s <image path>\n", argv[0]);
                exit(0);
        }

        img_path = argv[1];
        img = image_load(img_path);
        if (!img) {
                printf("Failed to load image: %s\n", img_path);
                load_failed = 1;
        }

        mainloop();
        return 0;
}
