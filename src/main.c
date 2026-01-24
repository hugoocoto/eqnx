#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "flag.h"
#include "keypress.h"
#include "plug_api.h"
#include "plug_co.h"
#include "wayland_client.h"

/* This plugin is the entry point of the program, it's the first and unique
 * plugin called from here */
#define INIT_PLUGIN "./plugins/plugin.so"

Plugin *p;

void
keypress_listener(Keypress kp)
{
        plug_send_kp_event(p, kp.sym, kp.mods);
}


int
loop(char *ppath)
{
        printf("(main.c: plugin -> %s)\n", ppath);
        p = plug_open(ppath);
        if (plug_exec(p)) return 1;
        keypress_add_listener(keypress_listener);

        for (;;) {
                if (wayland_dispatch_events()) {
                        printf("Wayland ask to close\n");
                        break;
                }
                wayland_present();
        }

        plug_release(p);
        plug_destroy(p);
        printf("(main.c) return\n");
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
                printf("Version: 0.0.0\n");
                exit(0);
        }

        if (wayland_init()) {
                printf("Can not open wayland display!\n");
                exit(1);
        }
        wayland_set_title("Eqnx");

        loop(ppath);

        wayland_cleanup();

        return 0;
}
