#include <assert.h>
#include <stdio.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../src/plug_api.h"

Plugin *child;

void
resize(int w, int h)
{
        printf("New size: %d %d\n", w, h);
}

void
kp_event(int sym, int mods)
{
        if (sym == 'A') printf("A pressed\n");
        if (sym == XKB_KEY_space) printf("Space pressed\n");
        plug_send_kp_event(child, sym, mods);
}

void
render()
{
        printf("PLUGIN: render\n");
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        printf("(Plugin: %s) Hello!\n", argv[0]);
        child = plug_run("./plugins/plugin2.so");
        mainloop();
        printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
