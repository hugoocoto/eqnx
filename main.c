#include "plug_api.h"
#include <dlfcn.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINICORO_IMPL
#include "minicoro.h"

#define EXPORTED // mark functions as part of the api
#define UNUSED(x) ((void) (x))
#define INIT_PLUGIN "plugin.so"

#define plugin_defaults                      \
        (Plugin)                             \
        {                                    \
                .main = plugin_default_main, \
        }

#define plugin_new() (memcpy(malloc(sizeof(Plugin)), \
                             &plugin_defaults,       \
                             sizeof(Plugin)))

typedef struct PlugUpwardsCall {
        enum {
                MAINLOOP = 8974, // just for debugging reasons (yes, for
                                 // debugging, trust me)
                RUN = 8975,
        } type;
        void *arg;
} PlugUpwardsCall;

EXPORTED Plugin *
child_run(char *plugpath)
{
        Plugin *plug;
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = RUN,
                .arg = plugpath,
        };
        assert(mco_push(mco_running(), &plugpath, sizeof(char *)) == MCO_SUCCESS);
        // printf("Push %p plug path\n", mco_running());
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        // printf("Push %p call\n", mco_running());
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
        assert(mco_pop(mco_running(), &plug, sizeof(Plugin *)) == MCO_SUCCESS);
        // printf("Pop %p plug addr\n", mco_running());
        return plug;
}

EXPORTED void
mainloop()
{
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = MAINLOOP,
        };
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        // printf("Push %p call\n", mco_running());
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
}

// Coroutine entry function.
static void
coro_entry(mco_coro *co)
{
        Plugin *plug = mco_get_user_data(co);
        int status = plug->main(1, (char *[]) { plug->name });
        printf("%s main returns %d\n", plug->name, status);
}


static int
plugin_default_main(int argc, char **argv)
{
        UNUSED(argc);
        UNUSED(argv);
        printf("Plugin has no main! Using default one.\n");
        return 0;
}

static void
plug_destroy(Plugin *p)
{
        if (!p) return;
        if (p->handle) dlclose(p->handle);
        if (p->co) assert(mco_destroy(p->co) == MCO_SUCCESS);
        free(p);
}

static void
plug_release(Plugin *p)
{
        if (!p) return;
        for (int i = 0; i < p->childs.count; i++) {
                plug_release(p->childs.data[i]);
        }
        assert(mco_resume(p->co) == MCO_SUCCESS);
        assert(mco_status(p->co) == MCO_DEAD);
}

static Plugin *
plug_open(const char *plugdir)
{
        Plugin *plug = plugin_new();
        void *handle = dlopen(plugdir, RTLD_NOW);
        if (!handle) {
                printf("Error: dlopen: %s\n", dlerror());
                plug_destroy(plug);
                return 0;
        }

        plug->handle = handle;
        plug->main = dlsym(handle, "main") ?: (void *) plug->main;

        char *s = strdup(plugdir);
        strncpy(plug->name, basename(s), sizeof plug->name - 1);
        free(s);

        // Init plugin corrutine
        mco_desc desc = mco_desc_init(coro_entry, 0);  // First initialize a `desc` object through `mco_desc_init`.
        desc.user_data = plug;                         // Configure `desc` fields when needed (e.g. customize user_data or allocation functions).
        mco_result res = mco_create(&plug->co, &desc); // Call `mco_create` with the output coroutine pointer and `desc` pointer.
        assert(res == MCO_SUCCESS);
        assert(mco_status(plug->co) == MCO_SUSPENDED); // The coroutine should be now in suspended state.
        return plug;
}

static void
plug_add_child(Plugin *parent, Plugin *child)
{
        assert(parent);
        assert(child);
        if (parent->childs.capacity <= parent->childs.count) {
                if (parent->childs.capacity == 0) {
                        parent->childs.capacity = 2;
                } else {
                        parent->childs.capacity *= 2;
                }
                parent->childs.data = realloc(parent->childs.data,
                                              sizeof(Plugin *) *
                                              parent->childs.capacity);
        }
        parent->childs.data[parent->childs.count] = child;
        parent->childs.count++;
}

static int
plug_run(Plugin *p)
{
        if (!p) return 1;

        assert(mco_resume(p->co) == MCO_SUCCESS);

        for (;;) {
                switch (mco_status(p->co)) {
                case MCO_DEAD:
                        assert(mco_destroy(p->co) == MCO_SUCCESS);
                        printf("Invalid plugin: Don't forget to call mainloop()!\n");
                        abort(); // Maybe not

                case MCO_SUSPENDED: {
                        PlugUpwardsCall call;
                        assert(mco_pop(p->co, &call, sizeof call) == MCO_SUCCESS);
                        // printf("Pop %p call\n", p->co);
                        switch (call.type) {
                        case RUN: {
                                char *plugdir;
                                assert(mco_pop(p->co, &plugdir, sizeof(char *)) == MCO_SUCCESS);
                                // printf("Pop %p plug path\n", p->co);
                                Plugin *plug = plug_open(plugdir);
                                plug_add_child(p, plug);
                                plug_run(plug);
                                assert(mco_push(p->co, &plug, sizeof(Plugin *)) == MCO_SUCCESS);
                                // printf("Push %p plug addr\n", p->co);
                                assert(mco_resume(p->co) == MCO_SUCCESS);
                        } break;

                        case MAINLOOP:
                                return 0;
                        default:
                                printf("Error %s:%d: unhandled branch %d!\n", __FILE__, __LINE__, call.type);
                                abort();
                        }
                } break;

                default:
                        printf("Error %s:%d: unhandled branch!\n", __FILE__, __LINE__);
                        abort();
                }
        }
        return 0;
}

int
main(int argc, char **argv)
{
        UNUSED(argc);
        UNUSED(argv);
        printf("(main.c)\n");
        Plugin *p = plug_open(INIT_PLUGIN);
        plug_run(p);
        printf("Press any key to terminate...");
        fflush(stdout);
        getchar();
        plug_release(p);
        plug_destroy(p);
        printf("(main.c) return\n");
        return 0;
}
