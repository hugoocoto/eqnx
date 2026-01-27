#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "draw.h"
#include "wayland_client.h"
#include "window.h"

int
window_resize(Window *window, int x, int y, int h, int w)
{
        assert(x == 0);
        assert(y == 0);
        window->h = h;
        window->w = w;
        window->gap = w;

        size_t total_elements = (size_t) h * w;
        if (window->capacity < total_elements) {
                if (window->capacity == 0) window->capacity = 64;
                while (window->capacity < total_elements) {
                        window->capacity *= 2;
                }
                window->buffer = realloc(window->buffer, window->capacity * sizeof(window->buffer[0]));
        }

        return window->buffer == NULL;
}

uint32_t
window_get_codepoint(Window *window, int c, int r)
{
        assert(c < window->w);
        assert(r < window->h);
        return window->buffer[r * window->gap + c].c;
}

struct Char3
window_get(Window *window, int c, int r)
{
        assert(c < window->w);
        assert(r < window->h);
        return window->buffer[r * window->gap + c];
}

void
window_set(Window *window, int c, int r, uint32_t cp, uint32_t fg, uint32_t bg)
{
        assert(r < window->h);
        assert(c < window->w);
        window->buffer[r * window->gap + c] = (struct Char3) { cp, fg, bg };
}

void
window_setall(Window *window, uint32_t cp, uint32_t fg, uint32_t bg)
{
        for (int r = 0; r < window->h; r++) {
                for (int c = 0; c < window->w; c++) {
                        window_set(window, c, r, cp, fg, bg);
                }
        }
}

Window *
window_create(int x, int y, int h, int w)
{
        Window *window = malloc(sizeof(Window));
        memcpy(window, &DEFAULT_WINDOW, sizeof(Window));
        assert(window_resize(window, x, y, h, w) == 0);
        return window;
}

int
window_resize_px(Window *window, int fb_h, int fb_w)
{
        assert(fb_w > 0 && fb_h > 0);

        Font *f = get_default_font();
        assert(f != NULL);

        int grid_height = f->l_h;
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&f->info, 'A', &ax, &lsb);
        int grid_width = roundf(ax * f->scale);

        assert(grid_height > 0 && grid_width > 0);

        int rows = fb_h / grid_height;
        int cols = fb_w / grid_width;

        return window_resize(window, 0, 0, rows, cols);
}

Window *
create_fullscreen_window()
{
        int fb_w = 0, fb_h = 0;

        fb_get_size(&fb_w, &fb_h);
        assert(fb_w > 0 && fb_h > 0);

        Font *f = get_default_font();
        assert(f != NULL);

        int grid_height = f->l_h;
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&f->info, 'A', &ax, &lsb);
        int grid_width = roundf(ax * f->scale);

        assert(grid_height > 0 && grid_width > 0);

        int rows = fb_h / grid_height;
        int cols = fb_w / grid_width;

        Window *win = window_create(0, 0, rows, cols);

        return win;
}
