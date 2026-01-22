#include "plug_api.h"
#include <dlfcn.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flag.h"

#define MINICORO_IMPL
#include "minicoro.h"

#define EXPORTED // mark functions as part of the api
#define UNUSED(x) ((void) (x))

/* This plugin is the entry point of the program, it's the first and unique
 * plugin called from here */
#define INIT_PLUGIN "./plugin.so"

#define plugin_defaults                      \
        (Plugin)                             \
        {                                    \
                .main = plugin_default_main, \
        }

#define plugin_new() (memcpy(malloc(sizeof(Plugin)), \
                             &plugin_defaults,       \
                             sizeof(Plugin)))

typedef struct PlugUpwardsCall {
        void *arg;
        enum {
                MAINLOOP,
                RUN,
        } type;
} PlugUpwardsCall;

EXPORTED Plugin *
plug_run(char *plugpath)
{
        Plugin *plug;
        PlugUpwardsCall call = (PlugUpwardsCall) {
                .type = RUN,
                .arg = plugpath,
        };

        assert(mco_push(mco_running(), &plugpath, sizeof(char *)) == MCO_SUCCESS);
        assert(mco_push(mco_running(), &call, sizeof call) == MCO_SUCCESS);
        assert(mco_yield(mco_running()) == MCO_SUCCESS);
        assert(mco_pop(mco_running(), &plug, sizeof(Plugin *)) == MCO_SUCCESS);
        return plug;
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
static void
coro_entry(mco_coro *co)
{
        Plugin *plug = mco_get_user_data(co);
        int status = plug->main(1, (char *[]) { plug->name });
        // main returns here
        UNUSED(status);
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
        for (int i = 0; i < p->children.count; i++) {
                plug_release(p->children.data[i]);
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
                fprintf(stderr, "Error: dlopen: %s\n", dlerror());
                plug_destroy(plug);
                return 0;
        }

        plug->handle = handle;
        plug->main = dlsym(handle, "main") ?: (void *) plug->main;

        char *s = strdup(plugdir);
        strncpy(plug->name, basename(s), sizeof plug->name - 1);
        free(s);

        // Init plugin corrutine
        mco_desc desc = mco_desc_init(coro_entry, 0);
        desc.user_data = plug;
        assert(mco_create(&plug->co, &desc) == MCO_SUCCESS);
        assert(mco_status(plug->co) == MCO_SUSPENDED);
        return plug;
}

static void
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
                parent->children.data = realloc(parent->children.data,
                                                sizeof(Plugin *) *
                                                parent->children.capacity);
        }
        parent->children.data[parent->children.count] = child;
        parent->children.count++;
}

static int
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
                        printf("Invalid plugin: Don't forget to call mainloop()!\n");
                        abort(); // Maybe not

                case MCO_SUSPENDED: {
                        PlugUpwardsCall call;
                        assert(mco_pop(p->co, &call, sizeof call) == MCO_SUCCESS);
                        switch (call.type) {
                        case RUN: {
                                char *plugdir;
                                assert(mco_pop(p->co, &plugdir, sizeof(char *)) == MCO_SUCCESS);
                                Plugin *plug = plug_open(plugdir);
                                if (!plug_exec(plug)) plug_add_child(p, plug);
                                assert(mco_push(p->co, &plug, sizeof(Plugin *)) == MCO_SUCCESS);
                                assert(mco_resume(p->co) == MCO_SUCCESS);
                        } break;

                        case MAINLOOP:
                                return 0;
                        default:
                                printf("Error %s:%d: unhandled branch (call type)%d!\n",
                                       __FILE__, __LINE__, call.type);
                                abort();
                        }
                } break;

                default:
                        printf("Error %s:%d: unhandled branch (co status)!\n",
                               __FILE__, __LINE__);
                        abort();
                }
        }
        return 0;
}

int
main(int argc, char **argv)
{
        char *v, *ppath;
        flag_program(.help = "Plugs by Hugo Coto");
        flag_add(&v, "--version", "-v", .help = "show version");
        flag_add(&ppath, "--plugin", "-p", .help = "init plugin", .defaults = INIT_PLUGIN, .nargs = 1);

        if (flag_parse(&argc, &argv)) {
                flag_show_help(STDOUT_FILENO);
                exit(1);
        } else if (v) {
                printf("Version: 0.0.0\n");
                exit(0);
        }

        printf("(main.c: plugin -> %s)\n", ppath);
        Plugin *p = plug_open(ppath);
        if (plug_exec(p)) return 1;
        printf("Press any key to terminate...");
        fflush(stdout);
        getchar();
        plug_release(p);
        plug_destroy(p);
        printf("(main.c) return\n");
        return 0;
}
