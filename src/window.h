#ifndef WINDOW_H_
#define WINDOW_H_ 1

#include <stddef.h>
#include <stdint.h>

typedef struct Window Window;

Window *window_create(int x, int y, int h, int w);
Window *create_fullscreen_window();
int window_resize(Window *, int x, int y, int h, int w);
int window_resize_px(Window *window, int x, int y, int w, int h);
void window_px_to_coords(int px, int py, int *x, int *y);

struct Char3 window_get(Window *window, int x, int y);
uint32_t window_get_codepoint(Window *window, int x, int y);

void window_set(Window *window, int x, int y, uint32_t c, uint32_t fg, uint32_t bg);
void window_setall(Window *window, uint32_t c, uint32_t fg, uint32_t bg);
void window_puts(Window *window, int x, int y, char *str, uint32_t fg, uint32_t bg);
void window_printf(Window *window, int x, int y, uint32_t fg, uint32_t bg, char *fmt, ...);

// Get a window representing part of window.
Window *window_cut(Window *window, int x, int y, int w, int h);

typedef struct WindowSharedBuffer {
        struct Char3 {
                // codepoint buffer (codepoint, fg and bg colors)
                uint32_t cp, fg, bg;
        } *buffer;
        size_t capacity; // buffer total capacity in bytes
        int gap;         // gap between rows (p = x * gap + y)
} WindowSharedBuffer;

typedef struct Window {
        int x, y; // start buffer offset - for windows that have the same buffer as the parent.
        int w, h;
        WindowSharedBuffer *shared;
        Window *parent; // parent window or null
} Window;

#define DEFAULT_WINDOW \
        (Window)       \
        {              \
                0,     \
        }

#endif
