#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window;

void
render()
{
        window_setall(self_window, ' ', RED, RED);
}

int
main(int argc, char **argv)
{
        mainloop();
        return 0;
}
