#ifndef PLUG_H_
#define PLUG_H_ 1

#include "draw.h"
#include "esx.h"
#include "event.h"
#include "window.h"

/* Plugin: Stores info and status related to a plugin (forward declared) */
typedef struct Plugin Plugin;

/* Note about plugin path (plugpath):
 *
 * If path contains a slash ("/"), then it is interpreted as a (relative or
 * absolute) pathname. Otherwise, the dynamic linker searches for the object
 * as follows:
 *
 *  - If, at the time that the program was started, the environment variable
 *  LD_LIBRARY_PATH was defined to contain a colon-separated list of
 *  directories, then these are searched. (As a security measure, this variable
 *  is ignored for set-user-ID and set-group-ID programs.)
 *
 *  - The directories /lib and /usr/lib are searched (in that order).
 *
 * If the object specified by path has dependencies on other shared objects,
 * then these are also automatically loaded by the dynamic linker using the same
 * rules. (This process may occur recursively, if those objects in turn have
 * dependencies, and so on.)
 */

/* Create a child plugin and execute it. This function returns once mainloop is
 * called by the child. It returns either the plugin structure or NULL, if
 * something went wrong. Argv is the array of startup values, argv[0] should be
 * equal to plugpath, and last value of argv have to be NULL. Argc is the number
 * of values in argv, also counting the null terminator.*/
Plugin *plug_run(char *plugpath, Window *, int argc, char **argv);

/* Enter plugin "mainloop". It's not a loop, but acts like one. This function
 * returns when the program has to end. Once this function is called, the plugin
 * is going to start receiving events. */
void mainloop();

// From plug_co.h
extern void plug_send_kp_event(Plugin *p, int sym, int mods);
extern void plug_send_resize_event(Plugin *p, int x, int y, int w, int h);
extern void plug_send_mouse_event(Plugin *p, Pointer_Event);
extern void plug_replace_img(Plugin *current, char *plugpath);
extern void ask_for_redraw();

// from wayland_client.h
extern int fb_capture(char *filename);

#define LIST_OF_CALLBACKS                    \
        X(int, main, int, char **)           \
        X(int, event, Event)                 \
        X(int, kp_event, int sym, int mods)  \
        X(int, pointer_event, Pointer_Event) \
        X(int, render)                       \
        X(int, resize, int x, int y, int w, int h)

#define X(ret, func, ...) ret (*func)(__VA_ARGS__);
typedef struct Plugin {
        char name[32];
        void *handle;
        Window *window;
        void *co;
        LIST_OF_CALLBACKS;
        struct {
                int capacity;
                int count;
                struct Plugin **data;
        } children;
        struct {
                int capacity;
                int count;
                char **data;
        } args;
} Plugin;
#undef X

#define mouse_event pointer_event

#endif // !PLUG_H_
