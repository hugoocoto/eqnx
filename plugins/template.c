#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/plug_api.h"

// Pure colors. uint32_t numbers
#define BLACK 0xFF000000
#define RED 0xFFFF0000
#define GREEN 0xFF00FF00
#define YELLOW 0xFFFFFF00
#define BLUE 0xFF0000FF
#define MAGENTA 0xFFFF00FF
#define CYAN 0xFF00FFFF
#define WHITE 0xFFFFFFFF

Window *self_window = NULL;
Plugin *self_plugin = NULL;

// Window has been resized! Top left forner is on x,y pixels, with w and h
// pixels width and height. Use window_px_to_coords() to get the window size
// in chars.
void
resize(int x, int y, int w, int h)
{
}

// Keypress event. A key has been pressed
void
kp_event(int sym, int mods)
{
}

// Pointer event. A mouse event (movement, click, scroll) has happened.
void
pointer_event(Pointer_Event e)
{
}

// This function is called when the program request a new frame. You can request
// a new frame from other functions using ask_for_redraw().
void
render()
{
        // don't forget to call draw_window(self_window);
}

// Main function - entry point.
int
main(int argc, char **argv)
{
        /* Your initializations here*/

        // Start receiving events. This is a blocking function.
        mainloop();

        /* Your deinitializations here*/

        return 0;
}
