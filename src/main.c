#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "da.h"
#include "draw.h"
#include "esx.h"
#include "event.h"
#include "flag.h"
#include "keypress.h"
#include "plug_co.h"
#include "wayland_client.h"
#include "window.h"

#ifndef VERSION
// version is defined in makefile
#define VERSION "unknown"
#endif

// like assert but returns x
#define inline_assert(x) ({__auto_type _x = x; assert(_x); _x; })

static Plugin *p;
static Window *window;
static bool need_redraw = true;
static jmp_buf safe_jmp_env;

struct {
        int capacity;
        int count;
        struct pollfd *data;
} fds;

void
listen_to_fd(int fd)
{
        da_append(&fds, (struct pollfd) { .fd = fd });
}

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
        plug_send_pointer_event(p, e);
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
        Esx_Program prog = { 0 };
        int argc;
        char **argv;

        if (esx_parse_file(ppath, &prog)) {
                printf("Can not load esx file %s\n", ppath);
                exit(1);
        }

        if (esx_to_args(prog, &argc, &argv)) {
                printf("Error while parsing arguments from esx file %s\n", ppath);
                exit(1);
        }

        for (int i = 0; i < argc; i++) {
                printf("%d: %s\n", i, argv[i]);
        }

        window = create_fullscreen_window();
        assert(window);

        if ((p = plug_open(argv[0], NULL, window)) == NULL) {
                printf("Plugin name can not be resolved\n");
                return 1;
        }

        for (int i = 0; i < argc; i++) {
                da_append(&p->args, argv[i]);
        }

        add_keypress_listener(keypress_listener);
        add_resize_listener(resize_listener);
        add_pointer_listener(pointer_listener);

        da_append(&fds, (struct pollfd) { .fd = wayland_get_fd() });

        if (setjmp(safe_jmp_env)) {
                goto loop;
        }

        send_resize_event();
        if (plug_exec(p)) return 1;
        send_resize_event();


loop:;
        while (!wayland_should_close()) {
                if (need_redraw) render_frame();

                while (wayland_prepare_read() != 0) {
                        if (wayland_dispatch_pending() < 0) {
                                perror("wayland_dispatch_pending");
                                return -1;
                        }
                }
                int ret = wayland_flush();

                for_da_each(e, fds)
                {
                        e->events = POLLIN;
                        e->revents = 0;
                }

                if (ret < 0 && errno == EAGAIN) {
                        fds.data[0].events |= POLLOUT;
                } else if (ret < 0) {
                        wayland_cancel_read();
                }

                if (poll(fds.data, fds.count, -1) < 0 && errno != EINTR) {
                        perror("poll");
                        wayland_cancel_read();
                        break;
                }

                if (fds.data[0].revents & POLLIN) {
                        if (wayland_read_events() < 0) {
                                perror("wayland_read_events");
                                break;
                        }
                        if (wayland_dispatch_pending() < 0) {
                                perror("dispatch_pending");
                                break;
                        }
                } else {
                        wayland_cancel_read();
                }
                if (fds.data[0].revents & POLLOUT) {
                        wayland_flush();
                }

                for (int i = 1; i < fds.count; i++) {
                        if (fds.data[i].revents & POLLIN) {
                                // fds.data[i].fd has input to read
                                // I have to notify
                        }
                }
                // print_fps();
        }

        plug_release(p);
        plug_destroy(p);
        return 0;
}

int
main(int argc, char **argv)
{
        char *v, *ppath;
        flag_program(.help = "eqnx by Hugo Coto");
        flag_add(&v, "--version", "-v", .help = "show version");
        flag_add(&ppath, "--program", "-p", .help = "ESX program to be loaded",
                 .nargs = 1, .required = 1);

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
