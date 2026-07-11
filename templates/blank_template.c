#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"

Window *self_window = NULL;
Plugin *self_plugin = NULL;

void
resize(int px, int py, int pw, int ph)
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
        mainloop();
        return 0;
}
