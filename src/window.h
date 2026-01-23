#ifndef WINDOW_H_
#define WINDOW_H_

#include <stddef.h>

typedef struct Window Window;

Window *window_create(int x, int y, int h, int w);       // create a window
int window_resize(Window *, int x, int y, int h, int w); // change window size
char *window_get(Window *window, int x, int y);          // get utf8 char at x,y
void window_set(Window *window, int x, int y, char *c);  // set utf8 char at x,y
void window_setall(Window *window, char *c);             // set all chars to utf8 char c

/* Window geometry (just for me)
 *
 *       (x,y) +----(w)----> (x,y+w)   0,0 +----------> +y
 *             |                           |
 *            (h)                          |
 *             |                           |
 *             v                           v
 *      (x+h,y)                          +x
 */

typedef struct Window {
        // int x, y;
        int w, h;
        char *buffer;    // UTF-8 (4 bytes) buffer
        size_t capacity; // buffer total capacity in bytes
        int gap;         // gap between rows (p = x * gap + y)

} Window;

#define DEFAULT_WINDOW \
        (Window)       \
        {              \
                0,     \
        }

#endif
