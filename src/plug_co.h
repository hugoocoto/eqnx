#ifndef PLUG_CO_H
#define PLUG_CO_H 1

#define OBSOLETE
#include "event.h"
#include "window.h"

#define LIST_OF_CALLBACKS                    \
        X(int, main, int, char **)           \
        X(int, event, Event)                 \
        X(int, kp_event, int sym, int mods)  \
        X(int, pointer_event, Pointer_Event) \
        X(int, render)                       \
        X(int, resize, int x, int y, int w, int h)

#define X(ret, func, ...) ret (*func)(__VA_ARGS__);
typedef struct Plugin {
        char name[32];
        void *handle;
        void *window;
        void *co;
        LIST_OF_CALLBACKS;
        struct {
                int capacity;
                int count;
                struct Plugin **data;
        } children;
        struct {
                int capacity;
                int count;
                char **data;
        } args;
} Plugin;
#undef X


void plug_destroy(Plugin *p);
void plug_release(Plugin *p);
Plugin *plug_open(char *plugdir, Plugin *plug_info, Window *window);
void plug_add_child(Plugin *parent, Plugin *child);
int plug_exec(Plugin *p);
void plug_send_kp_event(Plugin *p, int sym, int mods);
void plug_send_resize_event(Plugin *p, int x, int y, int w, int h);
void plug_send_pointer_event(Plugin *p, Pointer_Event);
void plug_render(Plugin *p);
void ask_for_redraw();
OBSOLETE Window *request_window();
OBSOLETE Plugin *request_plug_info();

void plug_replace_img(Plugin *current, char *plugpath);
void plug_safe_restart();
void send_resize_event();

#endif
