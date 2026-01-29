#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "flag.h"
#include "keypress.h"
#include "plug_api.h"
#include "plug_co.h"
#include "wayland_client.h"
#include "window.h"

#ifndef VERSION
// version is defined in makefile
#define VERSION "unknown"
#endif

extern void draw_window(Window *win);

/* This plugin is the entry point of the program, it's the first and unique
 * plugin called from here */
#define INIT_PLUGIN "./plugins/plugin.so"

static Plugin *p;
static bool need_redraw = true;

/* I use a lock on every callback call, so I make sure that plugin code is
 * sequential for avoiding run conditions.
 * */
static pthread_mutex_t single_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

void
render_frame()
{
        need_redraw = false;
        if (p->render) p->render();
        wayland_present();
}

void
ask_for_redraw()
{
        need_redraw = true;
}

static void
keypress_listener(Keypress kp)
{
        pthread_mutex_lock(&single_thread_mutex);
        plug_send_kp_event(p, kp.sym, kp.mods);
        pthread_mutex_unlock(&single_thread_mutex);
}

static void
resize_listener(int x, int y, int w, int h)
{
        pthread_mutex_lock(&single_thread_mutex);
        plug_send_resize_event(p, x, y, w, h);
        fb_clear(0xFF000000);
        render_frame();
        pthread_mutex_unlock(&single_thread_mutex);
}

static void
pointer_listener(Pointer_Event e)
{
        pthread_mutex_lock(&single_thread_mutex);
        plug_send_mouse_event(p, e);
        pthread_mutex_unlock(&single_thread_mutex);
}

static int
init_loop(char *ppath)
{
        // printf("(main.c: plugin -> %s)\n", ppath);

        p = plug_open(ppath);
        p->window = create_fullscreen_window();
        assert(p->window);

        if (plug_exec(p)) return 1;

        add_keypress_listener(keypress_listener);
        add_resize_listener(resize_listener);
        add_pointer_listener(pointer_listener);

        // unsigned long frame = 0;
        for (;;) {
                if (need_redraw) {
                        pthread_mutex_lock(&single_thread_mutex);
                        render_frame();
                        pthread_mutex_unlock(&single_thread_mutex);
                }

                if (wayland_dispatch_events()) {
                        break;
                }
                // printf("New frame %lu!\n", ++frame);
        }

        plug_release(p);
        plug_destroy(p);
        // printf("(main.c) return\n");
        return 0;
}

int
main(int argc, char **argv)
{
        char *v, *ppath;
        flag_program(.help = "Eqnx by Hugo Coto");
        flag_add(&v, "--version", "-v", .help = "show version");
        flag_add(&ppath, "--plugin", "-p", .help = "init plugin", .defaults = INIT_PLUGIN, .nargs = 1);

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        } else if (v) {
                printf("Eqnx version %s\n", VERSION);
                exit(0);
        }

        if (wayland_init()) {
                printf("Can not open wayland display!\n");
                exit(1);
        }

        wayland_set_title("Eqnx");
        init_loop(ppath);
        wayland_cleanup();

        return 0;
}
