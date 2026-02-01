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

/* On parallelism: As the plugin system is single-thread by design, calling api
 * functions from other threads may derive in unexpected results. It's permitted
 * to use more than one thread, once just the main one do the api calls. For
 * example, another thread can modify globals asynchronously, and the render
 * function can use this globals to update the window. Updating the window from
 * another thread is considered unexpected behaviour.
 *
 * TLDR: The api callbacks are defined in `plugin_api.h` and you can only call
 * api functions from this callbacks. Doing this from other threads is UB.
 */

// If you define this globals, they are assigned at plugin creation.
Window *self_window = NULL;
Plugin *self_plugin = NULL;

// This function is called when window is resized. Top left corner is on x,y
// pixels, with w and h pixels width and height. Use window_px_to_coords() to
// get the window size in chars.
void
resize(int x, int y, int w, int h)
{
        // (How to) get size in chars
        // int cx, cy, cw, ch;
        // window_px_to_coords(x, y, &cx, &cy);
        // window_px_to_coords(w, h, &cw, &ch);
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
        // Draw stuff in the window here
}

// Main function - entry point.
int
main(int argc, char **argv)
{
        /* Your initializations here */

        // Start receiving events. This is a blocking function.
        mainloop();

        /* Your deinitializations here */

        return 0;
}
