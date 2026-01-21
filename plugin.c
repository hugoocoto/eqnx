#include <assert.h>
#include <stdio.h>

#include "plug_api.h"

int
main(int argc, char **argv)
{
        assert(argc == 1);
        printf("(Plugin: %s) Hello!\n", argv[0]);
        plug_run("./plugin2.so");
        mainloop();
        printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
