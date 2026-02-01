#include <assert.h>
#include <ctype.h>
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
window_resize(Window *window, int x, int y, int w, int h)
{
        assert(x >= 0);
        assert(y >= 0);
        assert(w >= 0);
        assert(h >= 0);
        assert(window->shared);
        window->x = x;
        window->y = y;
        window->h = h;
        window->w = w;

        if (window->parent != NULL) return 0;

        window->shared->gap = w;
        size_t total_elements = (size_t) h * w;
        if (window->shared->capacity < total_elements) {
                if (window->shared->capacity == 0) window->shared->capacity = 64;
                while (window->shared->capacity < total_elements) {
                        window->shared->capacity *= 2;
                }
                window->shared->buffer =
                realloc(window->shared->buffer, window->shared->capacity * sizeof(window->shared->buffer[0]));
                memset(window->shared->buffer, 0, window->shared->capacity * sizeof(window->shared->buffer[0]));
                assert(window->shared->buffer != NULL);
        }

        return 0;
}

// uint32_t
// window_get_codepoint(Window *window, int c, int r)
// {
//         assert(c < window->w);
//         assert(r < window->h);
//         assert(c >= window->x);
//         assert(r >= window->y);
//         return window->buffer[r * window->gap + c].cp;
// }

void
window_puts(Window *window, int x, int y, uint32_t fg, uint32_t bg, char *str)
{
        if (!str) return;
        if (y < 0) return;
        size_t len = strlen(str);
        size_t i = 0;
        if (x < 0) i += -x;
        for (; i < len; i++) {
                if (!isprint(str[i])) continue;
                window_set(window, x + i, y, str[i], fg, bg);
        }
}

void
window_printf(Window *window, int x, int y, uint32_t fg, uint32_t bg, char *fmt, ...)
{
        if (!fmt) return;

        char buf[1024] = { 0 };
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf - 1, fmt, ap);
        va_end(ap);
        window_puts(window, x, y, fg, bg, buf);
}

struct Char3
window_get(Window *window, int c, int r)
{
        assert(c < window->w + window->x);
        assert(r < window->h + window->y);
        assert(c >= 0);
        assert(r >= 0);
        return window->shared->buffer[(r + window->y) * window->shared->gap +
                                      c + window->x];
}

void
window_set(Window *window, int c, int r, uint32_t cp, uint32_t fg, uint32_t bg)
{
        if (c >= window->w + window->x) return;
        if (r >= window->h + window->y) return;
        if (c < 0) return;
        if (r < 0) return;
        window->shared->buffer[(r + window->y) * window->shared->gap + c +
                               window->x] = (struct Char3) { cp, fg, bg };
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
        window->shared = calloc(1, sizeof(WindowSharedBuffer));
        assert(window->shared);
        assert(window_resize(window, x, y, h, w) == 0);
        return window;
}

void
window_px_to_coords(int px, int py, int *x, int *y)
{
        Font *f = get_default_font();
        assert(f != NULL);

        int grid_height = f->l_h;
        int ax, lsb;
        stbtt_GetCodepointHMetrics(&f->info, 'A', &ax, &lsb);
        int grid_width = roundf(ax * f->scale);

        assert(grid_height > 0 && grid_width > 0);

        if (y) *y = py / grid_height;
        if (x) *x = px / grid_width;
}

int
window_resize_px(Window *window, int px, int py, int pw, int ph)
{
        assert(pw > 0 && ph > 0);
        assert(px >= 0 && py >= 0);
        int x, y, w, h;
        window_px_to_coords(px, py, &x, &y);
        window_px_to_coords(pw, ph, &w, &h);

        return window_resize(window, x, y, w, h);
}

Window *
create_fullscreen_window()
{
        int fb_w = 0, fb_h = 0;

        fb_get_size(&fb_w, &fb_h);
        assert(fb_w > 0 && fb_h > 0);

        int rows, cols;
        window_px_to_coords(fb_w, fb_h, &rows, &cols);

        Window *win = window_create(0, 0, rows, cols);

        return win;
}

// Get a window representing part of window.
Window *
window_cut(Window *window, int x, int y, int w, int h)
{
        Window *child = malloc(sizeof(Window));
        assert(child);

        memcpy(child, window, sizeof(Window));

        child->parent = window;
        assert(child->shared->gap == child->parent->shared->gap);
        assert(x <= child->w && x >= child->x);
        assert(y <= child->h && y >= child->y);
        assert(x + w <= child->w);
        assert(y + h <= child->h);

        child->x = x;
        child->y = y;
        child->w = w;
        child->h = h;

        printf("Cut %p into %p (%d,%d -> +%d, +%d)\n", window, child, x, y, w, h);

        return child;
}
