#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
}

void
pointer_event(Pointer_Event e)
{
}

void
render()
{
}

int
main(int argc, char **argv)
{
        for (int i = 0; i < argc; i++) {
                printf("[%d] %s\n", argc, argv[i]);
        }
        mainloop();
        return 0;
}
