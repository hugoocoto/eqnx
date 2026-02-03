#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window;
Window *split_left;
static Window *split_right;
static Plugin *plug_left;
static Plugin *plug_right;

void
resize(int x, int y, int w, int h)
{
        int sw, cw;
        window_px_to_coords(w, 0, &cw, 0);
        window_coords_to_px(cw / 2, 0, &sw, 0);
        plug_send_resize_event(plug_left, x, y, sw, h);
        plug_send_resize_event(plug_right, x + sw, y, w - sw, h);
        ask_for_redraw();
}

void
kp_event(int sym, int mods)
{
        plug_send_kp_event(plug_left, sym, mods);
        plug_send_kp_event(plug_right, sym, mods);
}

void
pointer_event(Pointer_Event e)
{
        plug_send_pointer_event(plug_left, e);
        plug_send_pointer_event(plug_right, e);
}

void
render()
{
        plug_render(plug_left);
        plug_render(plug_right);
}

int
main(int argc, char **argv)
{
        printf("Plugin Arguments: ----------------\n");
        for (int i = 0; i < argc; i++) {
                printf("%d: %s\n", i, argv[i]);
        }
        printf("----------------------------------\n");

        int sw = self_window->w / 2;
        split_left = window_cut(self_window, self_window->x, self_window->y, sw, self_window->h);
        split_right = window_cut(self_window, self_window->x + sw, self_window->y, self_window->w - sw, self_window->h);
        assert(split_left);
        assert(split_right);

        if (argc != 3) {
                plug_left = plug_run("picker", split_left, 2, (char *[]) { "picker", NULL });
                plug_right = plug_run("picker", split_right, 2, (char *[]) { "picker", NULL });
                goto start;
        }

        Esx_Program prog;
        int e_argc;
        char **e_argv;

        if (esx_parse_string(argv[1], strlen(argv[1]), &prog) ||
            esx_to_args(prog, &e_argc, &e_argv)) {
                printf("Can not load esx file %s\n", argv[1]);
                exit(1);
        }

        if (e_argc < 1) {
                printf("Invalid parsing!\n");
                exit(1);
        }

        for (int i = 0; i < e_argc; i++) {
                printf("Plug Left arg %d: %s\n", i, e_argv[i]);
        }

        plug_left = plug_run(e_argv[0], split_left, e_argc, e_argv);
        if (!plug_left) {
                printf("Plug Left (%s) fail to load!\n", e_argv[0]);
        }


        if (esx_parse_string(argv[2], strlen(argv[2]), &prog) ||
            esx_to_args(prog, &e_argc, &e_argv)) {
                printf("Can not load esx file %s\n", argv[2]);
                exit(1);
        }

        if (e_argc < 1) {
                printf("Invalid parsing!\n");
                exit(1);
        }

        for (int i = 0; i < e_argc; i++) {
                printf("Plug Right arg %d: %s\n", i, e_argv[i]);
        }

        plug_right = plug_run(e_argv[0], split_right, e_argc, e_argv);
        if (!plug_right) {
                printf("Plug Right (%s) fail to load!\n", e_argv[0]);
                exit(1);
        }

start:
        ask_for_redraw();
        mainloop();
        return 0;
}
