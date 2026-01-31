#include <dlfcn.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "plug_api.h"
#include "plug_co.h"

#define MINICORO_IMPL
#include "../thirdparty/minicoro.h"

#define EXPORTED // mark functions as part of the api
#define UNUSED(x) ((void) (x))

int plugin_default_main(int argc, char **argv);

// copy plugin.so into a temp file and return the (strdup) name of the new file.
static char *
plugin_tmp_cpy(const char *path)
{
        char template[] = "/tmp/eqnx_plugin_XXXXXX";
        char buffer[1024];
        ssize_t n;
        ssize_t o;

        int fdout = mkstemp(template);
        int fdin = open(path, O_RDONLY);
        assert(fdout >= 0);
        assert(fdin >= 0);

        for (;;) {
                n = read(fdin, buffer, sizeof buffer);
                assert(n >= 0);
                if (n == 0) break;
                o = write(fdout, buffer, n);
                assert(n == o);
        }
        printf("Copied %s into %s\n", path, template);
        return strdup(template);
}

static Plugin *
plugin_new()
{
/*   */ #define X(_, name, ...) .name = NULL,
        return memcpy(malloc(sizeof(Plugin)),
                      &(Plugin) {
                      // default values
                      .window = NULL,
                      LIST_OF_CALLBACKS },
                      sizeof(Plugin));
/*   */ #undef X
}

typedef struct PlugUpwardsCall {
        void *arg;
        enum {
                MAINLOOP = 0xABC123, // avoid using other arguments as calls
                RUN,
                WINDOW_REQUEST,
                PLUG_INFO_REQUEST, // UGLY
        } type;
} PlugUpwardsCall;

EXPORTED void
plug_replace_img(Plugin *current, char *plugpath)
{
        /* Problem that this function tries to solve:
         *
         * When the plugin that is going to be executed is not known at
         * initialization of parent plugin, it's not possible to execute it
         * before mainloop returns. This function is going to use the corrutine
         * of the parent plugin to run the child plugin, as well as all use all
         * the plugin information.
         *
         * problem 2: How do I know the plugin that is being executed?
         * -> I'm passing it as an argument but I don't want to rely on user for
         *  this kind of things. I can write a trampoline for each callback that
         *  sets and resets the environment of the call, where I can store
         *  metadata as this plugin info and the window structs.
         */
        // return from mainloop
        mco_resume(current->co);
        mco_destroy(current->co);

        // Remplace symbols and name
        plug_open(plugpath, current, current->window);

        // Run main
        plug_exec(current);

        // return control to main thread after new plugin, at the same address
        // of current, reaches mainloop.
}

EXPORTED Plugin *
plug_run(char *plugpath, Window *w)
{
        Plugin *plug;
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = RUN,
                .arg = plugpath,
        };

        assert(mco_push(mco_running(), &w, sizeof(Window *)) == MCO_SUCCESS);
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
        assert(mco_pop(mco_running(), &plug, sizeof(Plugin *)) == MCO_SUCCESS);
        return plug;
}

EXPORTED OBSOLETE Plugin * // UGLY
request_plug_info()
{
        Plugin *p;
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = PLUG_INFO_REQUEST,
        };
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
        assert(mco_pop(mco_running(), &p, sizeof(Plugin *)) == MCO_SUCCESS);
        return p;
}

EXPORTED OBSOLETE Window *
request_window()
{
        Window *win;
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = WINDOW_REQUEST,
        };
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
        assert(mco_pop(mco_running(), &win, sizeof(Window *)) == MCO_SUCCESS);
        return win;
}

EXPORTED void
mainloop()
{
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = MAINLOOP,
        };
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
}

// Coroutine entry function (trampoline to main).
void
coro_entry(mco_coro *co)
{
        Plugin *plug = mco_get_user_data(co);
        if (!plug->main) plug->main = plugin_default_main;
        int status = plug->main(1, (char *[]) { plug->name });
        // main returns here
        UNUSED(status);
}

int
plugin_default_main(int argc, char **argv)
{
        UNUSED(argc);
        UNUSED(argv);
        printf("Plugin has no main! Using default one.\n");
        return 0;
}

void
plug_destroy(Plugin *p)
{
        if (!p) return;
        if (p->handle) dlclose(p->handle);
        if (p->co) assert(mco_destroy(p->co) == MCO_SUCCESS);
        free(p);
}

void
plug_release(Plugin *p)
{
        if (!p) return;
        for (int i = 0; i < p->children.count; i++) {
                plug_release(p->children.data[i]);
        }
        assert(mco_resume(p->co) == MCO_SUCCESS);
        assert(mco_status(p->co) == MCO_DEAD);
}

Plugin *
plug_open(const char *plugdir, Plugin *plug_info, Window *window)
{
        assert(window);
        Plugin *plug = plug_info ?: plugin_new();
        char *copy = plugin_tmp_cpy(plugdir);
        void *handle = dlopen(copy, RTLD_NOW);
        printf("dlopen %s (%s) succeed\n", plugdir, copy);
        free(copy);

        if (!handle) {
                fprintf(stderr, "Error: dlopen: %s\n", dlerror());
                plug_destroy(plug);
                return 0;
        }

        char *s = strdup(plugdir);
        strncpy(plug->name, basename(s), sizeof plug->name - 1);
        free(s);

        plug->handle = handle;
        plug->window = window;
        assert(plug->window);

        printf("loading plugin symbols (%s):\n", plug->name);
/*   */ #define X(_, name, ...)                                   \
        plug->name = dlsym(handle, #name) ?: (void *) plug->name; \
        printf("+ %s: %p\n", #name, plug->name);
        LIST_OF_CALLBACKS
/*   */ #undef X

        assert(plug->window);
        // Crazy shit
        Window **w = dlsym(handle, "self_window");
        if (w) *w = plug->window;
        Plugin **p = dlsym(handle, "self_plugin");
        if (p) *p = plug;

        assert(plug->window);
        // Init plugin corrutine
        mco_desc desc = mco_desc_init(coro_entry, 0);
        desc.user_data = plug;
        assert(mco_create((mco_coro **) &plug->co, &desc) == MCO_SUCCESS);
        assert(mco_status(plug->co) == MCO_SUSPENDED);
        assert(plug->window);
        return plug;
}

void
plug_send_mouse_event(Plugin *p, Pointer_Event e)
{
        if (!p) return;
        if (p->mouse_event) p->mouse_event(e);
}

void
plug_send_resize_event(Plugin *p, int x, int y, int w, int h)
{
        assert(x >= 0 && y >= 0 && w >= 0 && h >= 0);
        if (!p) return;
        window_resize_px(p->window, x, y, w, h);
        if (p->resize) p->resize(x, y, w, h);
}

void
plug_send_kp_event(Plugin *p, int sym, int mods)
{
        if (!p) return;
        if (p->kp_event) p->kp_event(sym, mods);
}

void
plug_add_child(Plugin *parent, Plugin *child)
{
        assert(parent);
        assert(child);
        while (parent->children.capacity <= parent->children.count) {
                if (parent->children.capacity == 0) {
                        parent->children.capacity = 2;
                } else {
                        parent->children.capacity *= 2;
                }
                parent->children.data =
                realloc(parent->children.data,
                        sizeof(Plugin *) * parent->children.capacity);
        }
        parent->children.data[parent->children.count] = child;
        parent->children.count++;
}

int
mco_suspended_handler(Plugin *p)
{
        PlugUpwardsCall call;
        assert(mco_pop(p->co, &call, sizeof call) == MCO_SUCCESS);

        switch (call.type) {
        case RUN: {
                Window *win;
                assert(mco_pop(p->co, &win, sizeof(Window *)) == MCO_SUCCESS);
                Plugin *plug = plug_open(call.arg, NULL, win);
                if (!plug_exec(plug)) plug_add_child(p, plug);
                assert(mco_push(p->co, &plug, sizeof(Plugin *)) == MCO_SUCCESS);
                assert(mco_resume(p->co) == MCO_SUCCESS);
        } break;

        case WINDOW_REQUEST: {
                assert(mco_push(p->co, &p->window, sizeof(Window *)) == MCO_SUCCESS);
                assert(mco_resume(p->co) == MCO_SUCCESS);
        } break;

        case PLUG_INFO_REQUEST: { // UGLY
                assert(mco_push(p->co, &p, sizeof(Plugin *)) == MCO_SUCCESS);
                assert(mco_resume(p->co) == MCO_SUCCESS);
        } break;

        case MAINLOOP:
                return 1;

        default:
                printf("Error %s:%d: unhandled branch (call type)%d!\n",
                       __FILE__, __LINE__, call.type);
                abort();
        }
        return 0;
}

int
plug_exec(Plugin *p)
{
        if (!p) return 1;
        assert(mco_resume(p->co) == MCO_SUCCESS);

        for (;;) {
                switch (mco_status(p->co)) {
                case MCO_DEAD:
                        /* This is executed only if a plugin main returns before
                         * mainloop() is called. */
                        assert(mco_destroy(p->co) == MCO_SUCCESS);
                        printf("Invalid plugin %s: Don't forget to call mainloop()!\n", p->name);
                        abort(); // Maybe not

                case MCO_SUSPENDED:
                        if (mco_suspended_handler(p)) return 0;
                        break;

                default:
                        printf("Error %s:%d: unhandled branch (co status)!\n",
                               __FILE__, __LINE__);
                        abort();
                }
        }
        return 0;
}
