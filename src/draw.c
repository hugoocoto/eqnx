#include <fontconfig/fontconfig.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "draw.h"
#include "wayland_client.h"
#include "window.h"

#include "../config.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

#define UNREACHABLE(...)                                                            \
        do {                                                                        \
                printf("Unreachable (%s:%d)" __VA_ARGS__ "\n", __FILE__, __LINE__); \
                abort();                                                            \
        } while (0)


uint32_t
utf8_to_codepoint(const char *str, int *consumed)
{
        if (!str || !*str) {
                if (consumed) *consumed = 0;
                return 0;
        }

        unsigned char b1 = str[0];

        // 1 byte: 0xxxxxxx (ASCII)
        if ((b1 & 0x80) == 0x00) {
                if (consumed) *consumed = 1;
                return b1;
        }

        // 2 bytes: 110xxxxx 10xxxxxx
        if ((b1 & 0xE0) == 0xC0) {
                if (consumed) *consumed = 2;
                unsigned char b2 = str[1];
                return ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        }

        // 3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
        if ((b1 & 0xF0) == 0xE0) {
                if (consumed) *consumed = 3;
                unsigned char b2 = str[1];
                unsigned char b3 = str[2];
                return ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        }

        // 4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        if ((b1 & 0xF8) == 0xF0) {
                if (consumed) *consumed = 4;
                unsigned char b2 = str[1];
                unsigned char b3 = str[2];
                unsigned char b4 = str[3];
                return ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
        }

        // invalid
        if (consumed) *consumed = 5;
        return 0xFFFD;
}

Font *
get_default_font()
{
        static Font *def = NULL;
        if (def) return def;

        char *path = fontpath ? strdup(fontpath) : font_find_by_name(fontname);
        if (path) def = load_font(path, fontsize);
        if (!def) {
                // rely on default monospace font
                if (path) free(path);
                path = font_find_by_name("monospace");
                def = load_font(path, fontsize);
        }
        printf("Font: %s loaded\n", path);
        free(path);
        assert(def);
        return def;
}


char *
font_find_by_name(const char *name)
{
        FcChar8 *route = 0;
        FcPattern *match = 0;
        FcPattern *pattern = 0;
        FcResult result;
        char *sroute = 0;

        if (!name) return NULL;

        if (!FcInit()) {
                printf("Error: Fontconfig FcInit fails\n");
                return NULL;
        }

        if ((pattern = FcNameParse((const FcChar8 *) name)) == NULL) {
                goto free_resources;
        }

        FcConfigSubstitute(NULL, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        if ((match = FcFontMatch(NULL, pattern, &result)) == NULL) {
                goto free_resources;
        }


        if (FcPatternGetString(match, FC_FILE, 0, &route) == FcResultMatch) {
                sroute = strdup((char *) route);
        }

free_resources:
        FcPatternDestroy(0); // assert that it is not going to explode on error
        FcPatternDestroy(pattern);
        FcPatternDestroy(match);
        FcFini();
        return sroute;
}

Font *
load_font(const char *path, int height)
{
        Font *f = calloc(1, sizeof *f);
        long size;

        FILE *fontFile = fopen(path, "rb");
        assert(fontFile);
        fseek(fontFile, 0, SEEK_END);
        size = ftell(fontFile);
        fseek(fontFile, 0, SEEK_SET);

        f->buffer = malloc(size);

        fread(f->buffer, size, 1, fontFile);
        fclose(fontFile);

        /* prepare font */
        if (!stbtt_InitFont(&f->info, f->buffer, 0)) {
                printf("failed\n");
        }

        f->l_h = height; /* line height */

        /* calculate font scaling */
        f->scale = stbtt_ScaleForPixelHeight(&f->info, f->l_h);
        stbtt_GetFontVMetrics(&f->info, &f->ascent, &f->descent, &f->lineGap);
        f->ascent = roundf(f->ascent * f->scale);
        f->descent = roundf(f->descent * f->scale);
        return f;
}

void
alpha_blend_inplace(uint32_t *dest, uint32_t new, float alpha, float alpha_range)
{
        int da, dr, dg, db;
        da = (*dest & 0xFF000000) >> 24;
        dr = (*dest & 0x00FF0000) >> 16;
        dg = (*dest & 0x0000FF00) >> 8;
        db = (*dest & 0x000000FF) >> 0;

        int na, nr, ng, nb;
        na = (new & 0xFF000000) >> 24;
        nr = (new & 0x00FF0000) >> 16;
        ng = (new & 0x0000FF00) >> 8;
        nb = (new & 0x000000FF) >> 0;

        int a, r, g, b;
        a = (da * (alpha_range - alpha) + na * alpha) / alpha_range;
        r = (dr * (alpha_range - alpha) + nr * alpha) / alpha_range;
        g = (dg * (alpha_range - alpha) + ng * alpha) / alpha_range;
        b = (db * (alpha_range - alpha) + nb * alpha) / alpha_range;

        *dest = a << 24 | r << 16 | g << 8 | b;
}


unsigned char *
get_fontcp(Font *f, uint32_t cp, int *xx, int *yy, int *bw, int *bh, int *ax, int *lsb)
{
/*   */ #define GLYPH_CACHE_SIZE 1024
        static struct {
                uint32_t codepoint;
                unsigned char *bitmap;
                stbtt_fontinfo info;
                int xx, yy, bw, bh, ax, lsb;
        } preload[GLYPH_CACHE_SIZE] = { 0 };

        uint32_t idx = cp % GLYPH_CACHE_SIZE;

        if (preload[idx].codepoint == cp && preload[idx].bitmap != NULL) {
                f->info = preload[idx].info;
                *xx = preload[idx].xx;
                *yy = preload[idx].yy;
                *bw = preload[idx].bw;
                *bh = preload[idx].bh;
                *ax = preload[idx].ax;
                *lsb = preload[idx].lsb;
                return preload[idx].bitmap;
        }

        if (preload[idx].bitmap != NULL) {
                stbtt_FreeBitmap(preload[idx].bitmap, NULL);
        }

        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointHMetrics(&f->info, cp, ax, lsb);
        stbtt_GetCodepointBitmapBox(&f->info, cp, f->scale, f->scale,
                                    &c_x1, &c_y1, &c_x2, &c_y2);
        *yy = f->ascent + c_y1;
        *xx = c_x1;

        unsigned char *bitmap = stbtt_GetCodepointBitmap(&f->info, f->scale, f->scale,
                                                         cp, bw, bh, 0, 0);

        preload[idx].codepoint = cp;
        preload[idx].bitmap = bitmap;
        preload[idx].info = f->info;
        preload[idx].xx = *xx;
        preload[idx].yy = *yy;
        preload[idx].bw = *bw;
        preload[idx].bh = *bh;
        preload[idx].ax = *ax;
        preload[idx].lsb = *lsb;

        return bitmap;
}

int
draw_cp(Font *f, int c, int r, struct Char3 sc)
{
        int xx, yy, ax, lsb, bw, bh;
        unsigned char *bitmap;

        bitmap = get_fontcp(f, sc.cp, &xx, &yy, &bw, &bh, &ax, &lsb);
        // int border_size = 2;
        // draw_clear_rectangle(c, r, get_grid_width(f), f->l_h, border_size, 0xFFFFFFFF);                                         // border
        // draw_rectangle(c + border_size, r + border_size, get_grid_width(f) - 2 * border_size, f->l_h - 2 * border_size, sc.bg); // background
        draw_rectangle(c, r, get_grid_width(f), f->l_h, sc.bg); // background
        print_bitmap(c + xx, r + yy, bitmap, bw, bh, sc.fg);
        return roundf(ax * f->scale);
}

void
draw_clear_rectangle(int x, int y, int w, int h, int size, uint32_t color)
{
        int fb_w = 0;
        int fb_h = 0;
        fb_get_size(&fb_w, &fb_h);
        assert(fb_w && fb_h);

        uint32_t *pixels = fb_get_active_data();
        assert(pixels);

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + w > fb_w) w = fb_w - x;
        if (y + h > fb_h) h = fb_h - y;

        for (int j = 0; j < h; j++) {
                for (int i = 0; i < w; i++) {
                        if (i < size || j < size ||
                            w - i <= size || h - j <= size) {
                                int pos = (y + j) * fb_w + (x + i);
                                pixels[pos] = color;
                        }
                }
        }
}

void
draw_rectangle(int x, int y, int w, int h, uint32_t color)
{
        int fb_w = 0, fb_h = 0;
        fb_get_size(&fb_w, &fb_h);
        uint32_t *pixels = fb_get_active_data();

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x + w > fb_w) w = fb_w - x;
        if (y + h > fb_h) h = fb_h - y;

        for (int r = 0; r < h; r++) {
                for (int c = 0; c < w; c++) {
                        int pos = (y + r) * fb_w + (x + c);
                        pixels[pos] = color;
                }
        }
}

void
print_bitmap(int cc, int rr, unsigned char *bitmap, int bw, int bh, uint32_t fg)
{
        int fb_w = 0, fb_h = 0;
        fb_get_size(&fb_w, &fb_h);
        uint32_t *pixels = fb_get_active_data();

        for (int r = 0; r < bh; r++) {
                for (int c = 0; c < bw; c++) {
                        int final_c = cc + c;
                        int final_r = rr + r;

                        if (final_c >= 0 && final_c < fb_w && final_r >= 0 && final_r < fb_h) {
                                alpha_blend_inplace(
                                &pixels[final_r * fb_w + final_c],
                                fg,
                                bitmap[r * bw + c], 256);
                        }
                }
        }
}

int
get_grid_width(Font *f)
{
        static Font *font = 0;
        static int grid_width;
        int ax, lsb;
        if (font) return grid_width;
        font = f;
        stbtt_GetCodepointHMetrics(&f->info, 'A', &ax, &lsb);
        grid_width = roundf(ax * f->scale);
        return grid_width;
}

void
draw_window(Window *win)
{
        printf("Drawing window\n");
        Font *f = get_default_font();
        int pixel_r = win->y * f->l_h;
        int pixel_c;

        // printf("Drawing window at (%d,%d +%d,+%d)\n",
               // win->x, win->y, win->w, win->h);

        int grid_width = get_grid_width(f);

        for (int r = 0; r < win->h; r++, pixel_r += f->l_h) {
                pixel_c = win->x * grid_width;

                for (int c = 0; c < win->w; c++, pixel_c += grid_width) {
                        struct Char3 sc = window_get(win, c, r);

                        if (sc.cp == 0) {
                                continue;
                        }

                        draw_cp(f, pixel_c, pixel_r, sc);
                }
        }
}

void
draw_clear_window(Window *window)
{
        window_setall(window, 0, 0xFF000000, 0xFF000000);
}

void
draw_clear_line(Window *window, int line)
{
        for (int i = 0; i < window->w; i++) {
                window_set(window, i, line, 0, 0xFF000000, 0xFF000000);
        }
}
