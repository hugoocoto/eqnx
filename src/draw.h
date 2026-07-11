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
void draw_rectangle(int px, int py, int pw, int ph, uint32_t color);
void draw_clear_rectangle(int px, int py, int pw, int ph, int size, uint32_t color);
void draw_window(Window *win);
uint32_t utf8_to_codepoint(const char *str, int *out_bytes_consumed);
void print_bitmap(int px, int py, unsigned char *bitmap, int bw, int bh, uint32_t color);

int get_grid_width(Font *f);
Font *get_default_font();
Font *load_font(const char *path, int height);
char *font_find_by_name(const char *name);

unsigned char *get_fontcp(Font *f, uint32_t cp, int *xx, int *yy, int *bw, int *bh, int *ax, int *lsb);
int draw_cp(Font *f, int px, int py, struct Char3);

typedef struct {
        uint32_t *pixels;  // ARGB 0xAARRGGBB
        int w, h;          // pixel dimensions
} Image;

Image *image_load(const char *path);
void   image_unload(Image *img); // optional — not required before process exit
void   draw_image(Image *img, int x, int y);
void   draw_image_scaled(Image *img, int x, int y, int w, int h);

#endif
