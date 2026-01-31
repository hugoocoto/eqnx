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

#define GRAY 0xFF666666
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define BLUE 0xFF0000FF
#define WHITE 0xFFFFFFFF

#ifndef VERSION
// version is defined in makefile
#define VERSION "unknown"
#endif

// like assert but returns x
#define inline_assert(x) ({__auto_type _x = x; assert(_x); _x; })

/* This plugin is the entry point of the program, it's the first and unique
 * plugin called from here */
#define INIT_PLUGIN "./plugins/picker.so"

static Plugin *p;
static Window *window;
static bool need_redraw = true;

/* I use a lock on every callback call, so I make sure that plugin code is
 * sequential for avoiding run conditions.
 * */
static pthread_mutex_t single_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

void
render_frame()
{
        printf("render frame\n");
        need_redraw = false;
        if (p->render) p->render();
        assert(p->window);
        draw_window(p->window);
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
        ask_for_redraw();
        pthread_mutex_unlock(&single_thread_mutex);
}

static void
pointer_listener(Pointer_Event e)
{
        pthread_mutex_lock(&single_thread_mutex);
        plug_send_mouse_event(p, e);
        pthread_mutex_unlock(&single_thread_mutex);
}

static void
send_resize_event()
{
        do {
                int w, h;
                fb_get_size(&w, &h);
                assert(w > 0 && h > 0);
                resize_listener(0, 0, w, h);
        } while (0);
}

static int
init_loop(char *ppath)
{
        window = create_fullscreen_window();
        assert(window);
        p = plug_open(ppath, NULL, window);
        assert(p);

        add_keypress_listener(keypress_listener);
        add_resize_listener(resize_listener);
        add_pointer_listener(pointer_listener);

        send_resize_event();

        if (plug_exec(p)) return 1;

        send_resize_event();

        for (;;) {
                if (wayland_dispatch_events()) {
                        break;
                }

                printf("wayland_dispatch_events returns\n");

                if (need_redraw) {
                        pthread_mutex_lock(&single_thread_mutex);
                        render_frame();
                        pthread_mutex_unlock(&single_thread_mutex);
                }
        }

        plug_release(p);
        plug_destroy(p);
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
