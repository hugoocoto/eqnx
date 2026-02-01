#include <assert.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
static jmp_buf safe_jmp_env;

void
render_frame()
{
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
        plug_send_kp_event(p, kp.sym, kp.mods);
}

static void
resize_listener(int x, int y, int w, int h)
{
        plug_send_resize_event(p, x, y, w, h);
        fb_clear(0xFF000000);
        ask_for_redraw();
}

static void
pointer_listener(Pointer_Event e)
{
        plug_send_mouse_event(p, e);
}

void
send_resize_event()
{
        do {
                int w, h;
                fb_get_size(&w, &h);
                assert(w > 0 && h > 0);
                resize_listener(0, 0, w, h);
        } while (0);
}

static void
print_fps()
{
        static time_t last_t = -1;
        static float fps = 0;
        time_t t;
        time(&t);
        fps++;
        if (last_t != t) {
                printf("FPS: %f\n", fps / (t - last_t));
                last_t = t;
                fps = 0;
        }
}

void
plug_safe_restart()
{
        longjmp(safe_jmp_env, 1);
}

static int
init_loop(char *ppath)
{
        if (setjmp(safe_jmp_env)) {
                puts("on safe jump cond");
                goto loop;
        }

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

loop:
        for (;;) {
                if (wayland_dispatch_events()) {
                        break;
                }

                if (need_redraw) {
                        render_frame();
                }
                print_fps();
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
