#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "window.h"

/* MISC */
void window_debug_term_print(Window *window); // print window buffer in stdout
static int get_uc_size(char *size);           // get utf-8 char size in bytes

Window *
window_create(int x, int y, int h, int w)
{
        Window *window = malloc(sizeof(Window));
        memcpy(window, &DEFAULT_WINDOW, sizeof(Window));
        assert(window_resize(window, x, y, h, w) == 0);
        return window;
}

// Returns 0 on success
int
window_resize(Window *window, int x, int y, int h, int w)
{
        // window->x = x;
        // window->y = y;
        assert(x == 0);
        assert(y == 0);
        window->h = h;
        window->w = w;
        window->gap = w;

        size_t needed_cap = h * w * 4;
        while (window->capacity < needed_cap) {
                if (window->capacity == 0) window->capacity = 64;
                window->capacity *= 2;
        }

        window->buffer = realloc(window->buffer, window->capacity);

        return window->buffer == NULL;
}

char *
window_get(Window *window, int x, int y)
{
        assert(x < window->h);
        assert(y < window->w);
        return &window->buffer[4 * (x * window->gap + y)];
}

static int
get_uc_size(char *size)
{
        if ((*size & 0x80) == 0x00) return 1;
        if ((*size & 0xE0) == 0xC0) return 2;
        if ((*size & 0xF0) == 0xE0) return 3;
        return 4;
}

void
window_set(Window *window, int x, int y, char *c)
{
        assert(x < window->h);
        assert(y < window->w);
        memcpy(&window->buffer[4 * (x * window->gap + y)], c, get_uc_size(c));
}

void
window_setall(Window *window, char *c)
{
        for (int i = 0; i < window->h; i++) {
                for (int j = 0; j < window->w; j++) {
                        window_set(window, i, j, c);
                }
        }
}

void
window_debug_term_print(Window *window)
{
        printf("\033[H\033[2J"); // clear screen
        for (int i = 0; i < window->h; i++) {
                for (int j = 0; j < window->w; j++) {
                        char *uc = window_get(window, i, j);
                        fwrite(uc, get_uc_size(uc), 1, stdout);
                }
                putchar(10);
        }
}

// int
// main()
// {
//         // setlocale(LC_ALL, "");
//
//         Window *window = window_create(0, 0, 10, 10);
//         window_setall(window, "ñ");
//         window_debug_term_print(window);
//         sleep(1);
//
//         window_resize(window, 0, 0, 5, 5);
//         window_setall(window, "0");
//         window_debug_term_print(window);
//         sleep(1);
//
//         window_resize(window, 0, 0, 10, 20);
//         window_setall(window, ".");
//         window_debug_term_print(window);
//         sleep(1);
//
//         window_resize(window, 0, 0, 80, 40);
//         window_setall(window, "?");
//         window_debug_term_print(window);
//         sleep(1);
// }
