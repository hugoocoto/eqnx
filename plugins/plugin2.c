#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

void
kp_event(int sym, int mods)
{
        // printf("kp event from plugin 2\n");
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        // printf("(Plugin: %s) Hello!\n", argv[0]);
        mainloop();
        // printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
