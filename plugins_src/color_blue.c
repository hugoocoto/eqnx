#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"
#include "theme.h"

Window *self_window;

void
render()
{
        window_setall(self_window, ' ', BLUE, BLUE);
}

int
main(int argc, char **argv)
{
        mainloop();
        return 0;
}
