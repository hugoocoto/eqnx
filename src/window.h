#ifndef WINDOW_H_
#define WINDOW_H_ 1

#include <stddef.h>
#include <stdint.h>

typedef struct Window Window;

Window *window_create(int x, int y, int h, int w);
Window *create_fullscreen_window();
int window_resize(Window *, int x, int y, int h, int w);
int window_resize_px(Window *window, int fb_h, int fb_w);
struct Char3 window_get(Window *window, int x, int y);
uint32_t window_get_codepoint(Window *window, int x, int y);
void window_set(Window *window, int x, int y, uint32_t c, uint32_t fg, uint32_t bg);
void window_setall(Window *window, uint32_t c, uint32_t fg, uint32_t bg);

typedef struct Window {
        int w, h;
        struct Char3 {
                // codepoint buffer (codepoint, fg and bg colors)
                uint32_t c, fg, bg;
        } *buffer;
        size_t capacity; // buffer total capacity in bytes
        int gap;         // gap between rows (p = x * gap + y)
} Window;

#define DEFAULT_WINDOW \
        (Window)       \
        {              \
                0,     \
        }

#endif
