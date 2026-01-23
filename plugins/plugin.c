#include <assert.h>
#include <stdio.h>

#include "../src/event.h"
#include "../src/plug_api.h"

void
event(Event event)
{
        printf("PLUGIN: event: %d\n", event.code);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        printf("(Plugin: %s) Hello!\n", argv[0]);
        plug_run("./plugins/plugin2.so");
        mainloop();
        printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
