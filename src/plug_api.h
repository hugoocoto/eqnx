#ifndef PLUG_H_
#define PLUG_H_ 1

#include "esx.h"
#include "event.h"
#include "window.h"

/* Plugin: Stores info and status related to a plugin */
typedef void Plugin;

/* Create a child plugin and execute it. This function returns once mainloop is
 * called by the child. It returns either the plugin structure or NULL, if
 * something went wrong. Argv is the array of startup values, argv[0] should be
 * equal to plugpath, and last value of argv have to be NULL. Argc is the number
 * of values in argv, also counting the null terminator.*/
Plugin *plug_run(char *plugpath, void *window, int argc, char **argv);

/* Enter plugin "mainloop". It's not a loop, but acts like one. This function
 * returns when the program has to end. Once this function is called, the plugin
 * is going to start receiving events. */
void mainloop();

/* plug_co.h */
extern void plug_send_kp_event(Plugin *p, int sym, int mods);
extern void plug_send_resize_event(Plugin *p, int x, int y, int w, int h);
extern void plug_send_pointer_event(Plugin *p, Pointer_Event);
extern void plug_replace_img(Plugin *current, char *plugpath);
extern void plug_render(Plugin *p);

extern void plug_replace_img(Plugin *current, char *plugpath);
extern void send_resize_event();

/* main.c */
extern void ask_for_redraw();
extern void listen_to_fd(int fd);

/* window.h */
extern void window_set(Window *window, int x, int y, uint32_t c, uint32_t fg, uint32_t bg);
extern void window_setall(Window *window, uint32_t c, uint32_t fg, uint32_t bg);
extern void window_puts(Window *window, int x, int y, uint32_t fg, uint32_t bg, char *str);
extern void window_printf(Window *window, int x, int y, uint32_t fg, uint32_t bg, char *fmt, ...);
extern void window_clear(Window *window, uint32_t fg, uint32_t bg);
extern void window_clear_line(Window *window, int line, uint32_t fg, uint32_t bg);
extern void window_px_to_coords(int px, int py, int *x, int *y);
extern void window_coords_to_px(int px, int py, int *x, int *y);

/* wayland_client.h */
extern int fb_capture(char *filename);

/* global_symbols.c */
void *symbol_get(char *symbol);
int symbol_add(char *symbol, void *value);


#define mouse_event pointer_event

#endif // !PLUG_H_
