#ifndef PLUG_CO_H
#define PLUG_CO_H 1

#include "plug_api.h"

void plug_destroy(Plugin *p);
void plug_release(Plugin *p);
Plugin *plug_open(const char *plugdir, Plugin *plug_info);
void plug_add_child(Plugin *parent, Plugin *child);
int plug_exec(Plugin *p);
void plug_send_kp_event(Plugin *p, int sym, int mods);
void plug_send_resize_event(Plugin *p, int x, int y, int w, int h);
void plug_send_mouse_event(Plugin *p, Pointer_Event);
void ask_for_redraw();
Window *request_window();
Plugin *request_plug_info();

void plug_replace_img(Plugin *current, char *plugpath);

#endif
