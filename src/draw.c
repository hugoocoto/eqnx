#include <fontconfig/fontconfig.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "config.h"
#include "draw.h"
#include "wayland_client.h"
#include "window.h"

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

        char *path = config.fontpath ? strdup(config.fontpath) : font_find_by_name(config.fontname);
        if (path) def = load_font(path, config.fontsize);
        if (!def) {
                // rely on default monospace font
                if (path) free(path);
                path = font_find_by_name("monospace");
                def = load_font(path, config.fontsize);
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
draw_cp(Font *f, int px, int py, struct Char3 sc)
{
        int xx, yy, ax, lsb, bw, bh;
        unsigned char *bitmap;

        bitmap = get_fontcp(f, sc.cp, &xx, &yy, &bw, &bh, &ax, &lsb);
        // int border_size = 2;
        // draw_clear_rectangle(px, py, get_grid_width(f), f->l_h, border_size, 0xFFFFFFFF);                                         // border
        // draw_rectangle(px + border_size, py + border_size, get_grid_width(f) - 2 * border_size, f->l_h - 2 * border_size, sc.bg); // background
        draw_rectangle(px, py, get_grid_width(f), f->l_h, sc.bg); // background
        print_bitmap(px + xx, py + yy, bitmap, bw, bh, sc.fg);
        return roundf(ax * f->scale);
}

void
draw_clear_rectangle(int px, int py, int pw, int ph, int size, uint32_t color)
{
        int fb_pw = 0;
        int fb_ph = 0;
        fb_get_size(&fb_pw, &fb_ph);
        assert(fb_pw && fb_ph);

        uint32_t *pixels = fb_get_active_data();
        assert(pixels);

        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px + pw > fb_pw) pw = fb_pw - px;
        if (py + ph > fb_ph) ph = fb_ph - py;

        for (int py_off = 0; py_off < ph; py_off++) {
                for (int px_off = 0; px_off < pw; px_off++) {
                        if (px_off < size || py_off < size ||
                            pw - px_off <= size || ph - py_off <= size) {
                                int pos = (py + py_off) * fb_pw + (px + px_off);
                                pixels[pos] = color;
                        }
                }
        }
}

void
draw_rectangle(int px, int py, int pw, int ph, uint32_t color)
{
        int fb_pw = 0, fb_ph = 0;
        fb_get_size(&fb_pw, &fb_ph);
        uint32_t *pixels = fb_get_active_data();

        if (px < 0) px = 0;
        if (py < 0) py = 0;
        if (px + pw > fb_pw) pw = fb_pw - px;
        if (py + ph > fb_ph) ph = fb_ph - py;

        for (int py_off = 0; py_off < ph; py_off++) {
                for (int px_off = 0; px_off < pw; px_off++) {
                        int pos = (py + py_off) * fb_pw + (px + px_off);
                        pixels[pos] = color;
                }
        }
}

void
print_bitmap(int px, int py, unsigned char *bitmap, int bw, int bh, uint32_t fg)
{
        int fb_pw = 0, fb_ph = 0;
        fb_get_size(&fb_pw, &fb_ph);
        uint32_t *pixels = fb_get_active_data();

        for (int r = 0; r < bh; r++) {
                for (int c = 0; c < bw; c++) {
                        int final_x = px + c;
                        int final_y = py + r;

                        if (final_x >= 0 && final_x < fb_pw && final_y >= 0 && final_y < fb_ph) {
                                alpha_blend_inplace(
                                &pixels[final_y * fb_pw + final_x],
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
        Font *f = get_default_font();
        int py = win->y * f->l_h;
        int px;
        int grid_width = get_grid_width(f);

        for (int y = 0; y < win->h; y++, py += f->l_h) {
                px = win->x * grid_width;
                for (int x = 0; x < win->w; x++, px += grid_width) {
                        struct Char3 sc = window_get(win, x, y);
                        if (sc.cp == 0) sc.cp = ' ';
                        draw_cp(f, px, py, sc);
                }
        }
}

void
window_clear(Window *window, uint32_t fg, uint32_t bg)
{
        window_setall(window, 0, fg, bg);
}

void
window_clear_line(Window *window, int y, uint32_t fg, uint32_t bg)
{
        for (int i = 0; i < window->w; i++) {
                window_set(window, i, y, 0, fg, bg);
        }
}
