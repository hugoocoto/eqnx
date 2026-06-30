#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
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

// program options
const char *v;
const char *b;
const char *force;
const char *ppath;
const char *cpath;
const char *root;

// global state
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
init_loop(const char *ppath)
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
build(const char *exec, const char *program, const char *config, const char *source, const char *root)
{
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) return 1;

        if (!source || !config || !program) {
                printf("Error: building requires program, config and source\n");
                return 1;
        }

        char *name;
        // copy program, trim at last dot and cut the dir
        char *base = strdup(program);
        char *c = strrchr(base, '.');
        if (c) *c = 0;
        asprintf(&name, "%s.sh", basename(base));
        free(base);

        FILE *f;
        if (!force) {
                f = fopen(name, "r");
                if (f != NULL) {
                        printf("File %s already exists\n", name);
                        fclose(f);
                        return 1;
                }
        }

        f = fopen(name, "w");
        if (f == NULL) {
                printf("Error opening %s\n", name);
                free(name);
                return 1;
        }

        // give run permissions
        struct stat st;
        if (!fstat(fileno(f), &st)) fchmod(fileno(f), st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);

        fprintf(f, "#!/bin/bash\n");
        fprintf(f, "# autogenerated eqnx run script for:\n");
        fprintf(f, "# - eqnx: %s\n", exec);
        fprintf(f, "# - prog: %s\n", program);
        fprintf(f, "# - conf: %s\n", config);
        fprintf(f, "# - psrc: %s\n", source);
        fprintf(f, "# - root: %s\n", root);
        fprintf(f, "\n");
        fprintf(f, "HERE=\"%s\"\n", cwd);
        fprintf(f, "EQNX=\"$(which \"%s\" || echo \"$HERE/%s\")\"\n", exec, exec);
        fprintf(f, "PROG=\"$HERE/%s\"\n", program);
        fprintf(f, "CONF=\"$HERE/%s\"\n", config);
        fprintf(f, "PSRC=\"$HERE/%s\"\n", source);
        fprintf(f, "\n");
        fprintf(f, "# Change cwd to root\n");
        fprintf(f, "cd \"%s\"\n", root);
        fprintf(f, "\n");
        fprintf(f, "# Run\n");
        fprintf(f, "\"$EQNX\" -p \"$PROG\" -c \"$CONF\" -s \"$PSRC\"\n");
        fclose(f);

        free(name);
        return 0;
}

// need to be read
const char *psrc;

int
main(int argc, char **argv)
{
        flag_program(.help = "~ eqnx by Hugo Coto");
        flag_add(&v, "--version", "-v", .help = "Show eqnx version");
        flag_add(&b, "--build", .help = "Generate a shell script that runs the program");
        flag_add(&force, "--force", "-f", .help = "Override build output if needed");
        flag_add(&root, "--root", "-r", .help = "Root used in build. Must be absolute", .nargs = 1, .defaults = "$HOME");
        flag_add(&ppath, "--prog", "-p", .help = "ESX program to be loaded", .nargs = 1);
        flag_add(&cpath, "--conf", "-c", .help = "Config file", .nargs = 1, .defaults = "config.lua");
        flag_add(&psrc, "--src", "-s", .help = "Plugin source path (terminated by '/')", .nargs = 1);

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        }

        if (v) {
                printf("Eqnx version %s\n", VERSION);
                exit(0);
        }
        if (!ppath) {
                printf("Eqnx program not set\n");
                exit(0);
        }
        if (psrc && psrc[strlen(psrc) - 1] != '/') {
                printf("Plugin source MUST end with a slash ('/')\n");
                exit(0);
        }

        if (b) {
                // program is set
                if (build(argv[0], ppath, cpath, psrc, root)) {
                        printf("Error while building:\n");
                        printf("- eqnx: %s\n", argv[0]);
                        printf("- prog: %s\n", ppath);
                        printf("- conf: %s\n", cpath);
                        printf("- psrc: %s\n", psrc);
                        printf("- root: %s\n", root);
                }
                exit(0);
        }

        if (wayland_init()) {
                printf("Can not open wayland display!\n");
                exit(1);
        }

        load_config(cpath);
        wayland_set_title("Eqnx");
        init_loop(ppath);
        wayland_cleanup();
        flag_free();

        return 0;
}
