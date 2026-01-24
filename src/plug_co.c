#include <dlfcn.h>
#include <libgen.h>

#include "plug_api.h"
#include "plug_co.h"

#define MINICORO_IMPL
#include "../thirdparty/minicoro.h"

#define EXPORTED // mark functions as part of the api
#define UNUSED(x) ((void) (x))

#define plugin_defaults                      \
        (Plugin)                             \
        {                                    \
                .main = plugin_default_main, \
                .render = NULL,              \
                .event = NULL,               \
                .kp_event = NULL,            \
                .resize = NULL,              \
                .window = NULL,              \
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
void
coro_entry(mco_coro *co)
{
        Plugin *plug = mco_get_user_data(co);
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
plug_open(const char *plugdir)
{
        Plugin *plug = plugin_new();
        void *handle = dlopen(plugdir, RTLD_NOW);
        if (!handle) {
                fprintf(stderr, "Error: dlopen: %s\n", dlerror());
                plug_destroy(plug);
                return 0;
        }

        char *s = strdup(plugdir);
        strncpy(plug->name, basename(s), sizeof plug->name - 1);
        free(s);

        plug->handle = handle;
        plug->main = dlsym(handle, "main") ?: (void *) plug->main;
        plug->event = dlsym(handle, "event") ?: (void *) plug->event;
        plug->render = dlsym(handle, "render") ?: (void *) plug->render;
        plug->resize = dlsym(handle, "resize") ?: (void *) plug->resize;
        plug->kp_event = dlsym(handle, "kp_event") ?: (void *) plug->kp_event;
        printf("plugin setup (%s):\n", plug->name);
        printf("+ main: %p\n", plug->main);
        printf("+ event: %p\n", plug->event);
        printf("+ render: %p\n", plug->render);
        printf("+ resize: %p\n", plug->resize);
        printf("+ kp_event: %p\n", plug->kp_event);

        // Init plugin corrutine
        mco_desc desc = mco_desc_init(coro_entry, 0);
        desc.user_data = plug;
        assert(mco_create(&plug->co, &desc) == MCO_SUCCESS);
        assert(mco_status(plug->co) == MCO_SUSPENDED);
        return plug;
}

void
plug_send_resize_event(Plugin *p, int w, int h)
{
        if (!p) return;
        if (p->resize) p->resize(w, h);
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
                char *plugdir;
                assert(mco_pop(p->co, &plugdir, sizeof(char *)) == MCO_SUCCESS);
                Plugin *plug = plug_open(plugdir);
                if (!plug_exec(plug)) plug_add_child(p, plug);
                assert(mco_push(p->co, &plug, sizeof(Plugin *)) == MCO_SUCCESS);
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
