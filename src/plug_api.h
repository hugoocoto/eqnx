#ifndef PLUG_H_
#define PLUG_H_ 1

#include "../thirdparty/minicoro.h"

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
 * something went wrong. */
Plugin *plug_run(char *plugpath);

/* Enter plugin "mainloop". It's not a loop, but acts like one. This function
 * returns when the program has to end. Once this function is called, the plugin
 * is going to start receiving events. */
void mainloop();

/* How to create a plugin:
 *
 * First, create a C program (or any other compiled language that supports C
 * shared library abi, bindings are not implemented yet).
 *
 * This program has to have a `int main(int, char**)` function, as any C
 * program. This function is the plugin entry point. It should be used to
 * initialize things, then run `mainloop()` and then deinitialize (or destroy)
 * those things (as any C program). See below for an example.
 *
 * Then, you can define other functions like `event(Event event)` (not yet
 * implemented, but it's the idea) to catch events. Plugins have to work as
 * action-reaction engines, in the sense that they can't have loop-like logic
 * (because mainloop() blocks the program thread until the program has to exit).
 *
 * All the logic related to this API runs on a single thread, it's not thread
 * safe, so any logic implemented in another thread should not interact with
 * this API. All the API calls have to be done from the main thread.
 */

/* Example: plugin.c
 * This program is a plugin that prints its status and has a child.
 *
 * ```c
 * #include "plug_api.h"
 *
 * int
 * main(int argc, char **argv)
 * {
 *         assert(argc == 1);
 *         printf("(Plugin: %s) Hello!\n", argv[0]);
 *         plug_run("./plugin2.so");
 *         mainloop();
 *         printf("(Plugin: %s) returns\n", argv[0]);
 *         return 0;
 * }
 * ```
 */

// From plug_co.h
extern void plug_send_kp_event(Plugin *p, int sym, int mods);

/* Info about plugins. You don't have to touch anything from here */
typedef struct Plugin {
        int (*main)(int, char **);
        int (*event)(Event);
        int (*kp_event)(int sym, int mods);
        int (*render)(Window);
        char name[32];
        void *handle;
        mco_coro *co;
        struct {
                struct Plugin **data;
                int capacity;
                int count;
        } children;
} Plugin;

#endif // !PLUG_H_
