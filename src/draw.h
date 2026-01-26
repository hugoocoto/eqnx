#ifndef DRAW_H_
#define DRAW_H_

#include "../thirdparty/stb_truetype.h"
#include "wayland_client.h"
#include "window.h"

typedef struct {
        unsigned char *buffer;
        stbtt_fontinfo info;
        int ascent, descent, lineGap;
        int l_h; /* line height */
        float scale;
} Font;

Font *load_font(const char *path, int size);
void draw_text(Font *f, int x, int y, const char *text, uint32_t color);
void draw_rectangle(int x, int y, int w, int h, uint32_t color);
void draw_clear_rectangle(int x, int y, int w, int h, int size, uint32_t color);
void draw_text(Font *f, int x, int y, const char *text, uint32_t color);
void draw_clear_rectangle(int x, int y, int w, int h, int size, uint32_t color);
void draw_rectangle(int x, int y, int w, int h, uint32_t color);
void draw_window(Window *win);
uint32_t utf8_to_codepoint(const char *str, int *out_bytes_consumed);
void print_bitmap(int x, int y, unsigned char *bitmap, int bw, int bh, uint32_t color);

Font *get_default_font();
Font *load_font(const char *path, int height);
char *font_find_by_name(const char *name);

unsigned char *get_fontcp(Font *f, uint32_t cp, int *xx, int *yy, int *bw, int *bh, int *ax, int *lsb);
int draw_cp(Font *f, int x, int y, const uint32_t cp, uint32_t color);

#endif
