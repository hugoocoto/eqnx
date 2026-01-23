#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"

int
main(int argc, char **argv)
{
        assert(argc == 1);
        printf("(Plugin: %s) Hello!\n", argv[0]);
        mainloop();
        printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
