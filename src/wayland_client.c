#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../wayland-protocol/xdg-shell-client-protocol.h"
#include "draw.h"
#include "event.h"
#include "keypress.h"
#include "wayland_client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"

#define TITLE "Untitled"

static struct wl_compositor *compositor;
static struct wl_display *display;
static struct wl_keyboard *keyboard;
static struct wl_pointer *pointer;
static struct wl_registry *registry;
static struct wl_seat *seat;
static struct wl_surface *surface;
static struct xdg_surface *xdg_surface;
static struct xdg_toplevel *xdg_toplevel;
static struct xdg_wm_base *xdg_wm_base;
static struct xkb_context *xkb_context;
static struct xkb_keymap *xkb_keymap;
static struct xkb_state *xkb_state;
static struct wl_shm *shm;
static int32_t current_output_scale = 1;
static struct wl_output *global_output = NULL;

static bool should_quit = false;
static int pending_height = 0;
static int pending_width = 0;
static double pointer_x = .0;
static double pointer_y = .0;
static double pending_pointer_x;
static double pending_pointer_y;
static bool has_pending_pointer = false;
static char *current_title = NULL;
static bool init = 0;

struct Framebuffer {
        struct wl_buffer *buffers[2];
        uint32_t *data;
        size_t capacity;
        int fd;
        int width, height;
        int stride;
        int current_idx;
} screen_fb;

static int
create_shm_file(off_t size)
{
        int fd = memfd_create("wl-eqnx", FD_CLOEXEC);
        if (fd < 0) return -1;
        if (ftruncate(fd, size) < 0) {
                close(fd);
                return -1;
        }
        return fd;
}

static void
fb_destroy()
{
        if (screen_fb.buffers[0]) wl_buffer_destroy(screen_fb.buffers[0]);
        if (screen_fb.buffers[1]) wl_buffer_destroy(screen_fb.buffers[1]);
        screen_fb.buffers[0] = NULL;
        screen_fb.buffers[1] = NULL;

        if (screen_fb.data) munmap(screen_fb.data, screen_fb.capacity);
        screen_fb.data = NULL;

        if (screen_fb.fd >= 0) close(screen_fb.fd);
        screen_fb.fd = -1;
}

static int
init_buffers(int w, int h, int stride)
{
        size_t page_size = stride * h;
        size_t total_size = page_size * 2;

        if (screen_fb.capacity < total_size || screen_fb.fd < 0) {
                fb_destroy();
                screen_fb.fd = create_shm_file(total_size);
                if (screen_fb.fd < 0) return -1;

                screen_fb.data = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, screen_fb.fd, 0);
                if (screen_fb.data == MAP_FAILED) {
                        close(screen_fb.fd);
                        return -1;
                }
                screen_fb.capacity = total_size;

        } else {
                if (screen_fb.buffers[0]) wl_buffer_destroy(screen_fb.buffers[0]);
                if (screen_fb.buffers[1]) wl_buffer_destroy(screen_fb.buffers[1]);
        }

        struct wl_shm_pool *pool = wl_shm_create_pool(shm, screen_fb.fd, screen_fb.capacity);

        screen_fb.buffers[0] = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_ARGB8888);
        screen_fb.buffers[1] = wl_shm_pool_create_buffer(pool, page_size, w, h, stride, WL_SHM_FORMAT_ARGB8888);

        wl_shm_pool_destroy(pool);
        return 0;
}

static int
fb_create(int w, int h)
{
        screen_fb = (struct Framebuffer) {
                .width = w,
                .height = h,
                .stride = w * 4, // ARGB
                .fd = -1,
        };

        return init_buffers(w, h, screen_fb.stride);
}

static int
fb_resize(int w, int h)
{
        if (w <= 0 || h <= 0) return 1;
        if (w == screen_fb.width && h == screen_fb.height) return 0;

        screen_fb.width = w;
        screen_fb.height = h;
        screen_fb.stride = w * 4;

        return init_buffers(w, h, screen_fb.stride);
}

// swap RR and BB channels
void
swizzling()
{
        uint32_t *buffer = fb_get_unactive_data();
        for (int i = 0; i < screen_fb.height * screen_fb.width; i++) {
                uint32_t ag = buffer[i] & 0xFF00FF00;
                uint32_t r = (buffer[i] & 0x00FF0000) >> 16;
                uint32_t b = (buffer[i] & 0x000000FF) << 16;
                buffer[i] = ag | r | b;
        }
}

// screen-capture the framebuffer. Returns the same as stbi_write_png.
int
fb_capture(char *filename)
{
        swizzling();
        int s = stbi_write_png(filename, screen_fb.width, screen_fb.height,
                               4, fb_get_unactive_data(), screen_fb.stride);
        swizzling();
        return s;
}

void
fb_get_size(int *w, int *h)
{
        *w = screen_fb.width;
        *h = screen_fb.height;
}

uint32_t *
fb_get_unactive_data()
{
        size_t offset_pixels = (1 - screen_fb.current_idx) *
                               (screen_fb.width * screen_fb.height);
        return screen_fb.data + offset_pixels;
}

uint32_t *
fb_get_active_data()
{
        size_t offset_pixels = screen_fb.current_idx * (screen_fb.width * screen_fb.height);
        return screen_fb.data + offset_pixels;
}

int
fb_clear(uint32_t color)
{
        uint32_t *pixels = fb_get_active_data();
        size_t count = screen_fb.width * screen_fb.height;
        for (size_t i = 0; i < count; ++i) {
                pixels[i] = color;
        }
        return 0;
}

void
fb_swap()
{
        screen_fb.current_idx = 1 - screen_fb.current_idx;
}

struct wl_buffer *
fb_get_ready_buffer()
{
        return screen_fb.buffers[screen_fb.current_idx];
}

static void
pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
        if (has_pending_pointer) {
                pointer_x = pending_pointer_x;
                pointer_y = pending_pointer_y;
        }

        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Move,
        .px = pointer_x,
        .py = pointer_y,
        .x = pointer_x / get_grid_width(f),
        .y = pointer_y / f->l_h,
        });
}

static void
pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Enter,
        });
}

static void
pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface)
{
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Leave,
        });
}

static void
pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
        pending_pointer_x = wl_fixed_to_double(surface_x);
        pending_pointer_y = wl_fixed_to_double(surface_y);
        has_pending_pointer = true;
}

static void
pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = state == WL_POINTER_BUTTON_STATE_PRESSED ? Pointer_Press :
                                                           Pointer_Release,
        .px = pointer_x,
        .py = pointer_y,
        .x = pointer_x / get_grid_width(f),
        .y = pointer_y / f->l_h,
        .btn = button,
        });
}

static void
pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Scroll,
        .px = pointer_x,
        .py = pointer_y,
        .x = pointer_x / get_grid_width(f),
        .y = pointer_y / f->l_h,
        .scroll = value,
        });
}

void
pointer_axis_relative_direction(void *data, struct wl_pointer *wl_pointer, uint32_t axis, uint32_t direction)
{
        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Scroll_Relative,
        .px = pointer_x,
        .py = pointer_y,
        .x = pointer_x / get_grid_width(f),
        .y = pointer_y / f->l_h,
        .scroll = axis,
        .direction = direction,
        });
}

static void
pointer_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)
{
        /* Opcional: Manejar la fuente del scroll (rueda, dedo, continuo) */
}

static void
pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)
{
        /* Opcional: Manejar el fin de la inercia del scroll */
}

static void
pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete)
{
        /* Opcional: Manejar pasos discretos de la rueda del ratón */
}

static const struct wl_pointer_listener pointer_listener = {
        .enter = pointer_enter,
        .leave = pointer_leave,
        .motion = pointer_motion,
        .button = pointer_button,
        .axis = pointer_axis,
        .frame = pointer_frame,
        .axis_source = pointer_axis_source,
        .axis_stop = pointer_axis_stop,
        .axis_discrete = pointer_axis_discrete,
};

static void
keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
                printf("Invalid keyboard format: %d\n", format);
                close(fd);
                return;
        }

        char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        if (map_shm == MAP_FAILED) {
                close(fd);
                return;
        }

        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_state) xkb_state_unref(xkb_state);

        xkb_keymap = xkb_keymap_new_from_string(xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

        munmap(map_shm, size);
        close(fd);

        if (!xkb_keymap) {
                printf("xkb_keymap compilation error\n");
                return;
        }

        xkb_state = xkb_state_new(xkb_keymap);
}

static void
keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
        // printf("Keyboard focus enters window\n");
}

static void
keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface)
{
        // printf("Keyboard focus leaves window\n");
}

static void
keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
        static int mods = 0;
        uint32_t keycode = key + 8;
        xkb_keysym_t sym = xkb_state_key_get_one_sym(xkb_state, keycode);

        if (sym == XKB_KEY_NoSymbol) {
                printf("sym = XKB_KEY_NoSymbol\n");
                return;
        }

        switch (state) {
        case WL_KEYBOARD_KEY_STATE_RELEASED:
                switch (sym) {
                case XKB_KEY_Shift_L: mods &= ~Mod_Shift_L; return;
                case XKB_KEY_Shift_R: mods &= ~Mod_Shift_R; return;
                case XKB_KEY_Control_L: mods &= ~Mod_Control_L; return;
                case XKB_KEY_Control_R: mods &= ~Mod_Control_R; return;
                case XKB_KEY_Caps_Lock: mods &= ~Mod_Caps_Lock; return;
                case XKB_KEY_Shift_Lock: mods &= ~Mod_Shift_Lock; return;
                case XKB_KEY_Meta_L: mods &= ~Mod_Meta_L; return;
                case XKB_KEY_Meta_R: mods &= ~Mod_Meta_R; return;
                case XKB_KEY_Alt_L: mods &= ~Mod_Alt_L; return;
                case XKB_KEY_Alt_R: mods &= ~Mod_Alt_R; return;
                case XKB_KEY_Super_L: mods &= ~Mod_Super_L; return;
                case XKB_KEY_Super_R: mods &= ~Mod_Super_R; return;
                case XKB_KEY_Hyper_L: mods &= ~Mod_Hyper_L; return;
                case XKB_KEY_Hyper_R: mods &= ~Mod_Hyper_R; return;
                }
                break;

        case WL_KEYBOARD_KEY_STATE_PRESSED:
                switch (sym) {
                default: {
                        uint32_t codepoint = xkb_state_key_get_utf32(xkb_state, keycode);
                        register_keypress(sym, mods, codepoint);
                } break;
                case XKB_KEY_Shift_L: mods |= Mod_Shift_L; return;
                case XKB_KEY_Shift_R: mods |= Mod_Shift_R; return;
                case XKB_KEY_Control_L: mods |= Mod_Control_L; return;
                case XKB_KEY_Control_R: mods |= Mod_Control_R; return;
                case XKB_KEY_Caps_Lock: mods |= Mod_Caps_Lock; return;
                case XKB_KEY_Shift_Lock: mods |= Mod_Shift_Lock; return;
                case XKB_KEY_Meta_L: mods |= Mod_Meta_L; return;
                case XKB_KEY_Meta_R: mods |= Mod_Meta_R; return;
                case XKB_KEY_Alt_L: mods |= Mod_Alt_L; return;
                case XKB_KEY_Alt_R: mods |= Mod_Alt_R; return;
                case XKB_KEY_Super_L: mods |= Mod_Super_L; return;
                case XKB_KEY_Super_R: mods |= Mod_Super_R; return;
                case XKB_KEY_Hyper_L: mods |= Mod_Hyper_L; return;
                case XKB_KEY_Hyper_R: mods |= Mod_Hyper_R; return;
                }
                break;

        case WL_KEYBOARD_KEY_STATE_REPEATED: break;
        }
}

static void
keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
        if (!xkb_state) return;
        xkb_state_update_mask(xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

static void
keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
        .keymap = keyboard_keymap,
        .enter = keyboard_enter,
        .leave = keyboard_leave,
        .key = keyboard_key,
        .modifiers = keyboard_modifiers,
        .repeat_info = keyboard_repeat_info,
};

static void
seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
        bool has_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
        bool has_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

        if (has_pointer && !pointer) {
                pointer = wl_seat_get_pointer(wl_seat);
                wl_pointer_add_listener(pointer, &pointer_listener, NULL);
        } else if (!has_pointer && pointer) {
                wl_pointer_release(pointer);
                pointer = NULL;
        }

        if (has_keyboard && !keyboard) {
                keyboard = wl_seat_get_keyboard(wl_seat);
                wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
        } else if (!has_keyboard && keyboard) {
                wl_keyboard_release(keyboard);
                keyboard = NULL;
        }
}

static void
seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
}

static const struct wl_seat_listener seat_listener = {
        .capabilities = seat_capabilities,
        .name = seat_name,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
        xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
        .ping = xdg_wm_base_ping,
};

static void
xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
        int new_w = (pending_width > 0) ? pending_width : screen_fb.width;
        int new_h = (pending_height > 0) ? pending_height : screen_fb.height;

        if (new_w != screen_fb.width || new_h != screen_fb.height) {
                fb_resize(new_w, new_h);
                notify_resize_event(new_w, new_h);
        }

        xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
        .configure = xdg_surface_configure,
};


static void
xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h, struct wl_array *states)
{
        enum xdg_toplevel_state *state;
        wl_array_for_each(state, states)
        {
                switch (*state) {
                case XDG_TOPLEVEL_STATE_MAXIMIZED:
                case XDG_TOPLEVEL_STATE_FULLSCREEN:
                case XDG_TOPLEVEL_STATE_RESIZING:
                case XDG_TOPLEVEL_STATE_ACTIVATED:
                case XDG_TOPLEVEL_STATE_TILED_LEFT:
                case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                case XDG_TOPLEVEL_STATE_TILED_TOP:
                case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                case XDG_TOPLEVEL_STATE_SUSPENDED:
                case XDG_TOPLEVEL_STATE_CONSTRAINED_LEFT:
                case XDG_TOPLEVEL_STATE_CONSTRAINED_RIGHT:
                case XDG_TOPLEVEL_STATE_CONSTRAINED_TOP:
                case XDG_TOPLEVEL_STATE_CONSTRAINED_BOTTOM: break;
                }
        }

        if (w > 0 && h > 0) {
                pending_width = w;
                pending_height = h;
        } else {
                pending_width = 0;
                pending_height = 0;
        }
}

static void
xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
        // printf("xdg toplevel close event\n");
        should_quit = true;
}

static void
xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel,
                              int32_t width, int32_t height)
{
        /* Opcional: Respetar estos límites si no estamos en fullscreen */
        /* Si width/height son 0, no hay límite */
}

static void
xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel,
                             struct wl_array *capabilities)
{
        /* Opcional: Iterar el array para saber si podemos minimizar/maximizar */
        /* uint32_t *cap;
           wl_array_for_each(cap, capabilities) { ... } */
}

/* Estructura actualizada para xdg_shell moderno */
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        .configure = xdg_toplevel_configure,
        .close = xdg_toplevel_close,
        .configure_bounds = xdg_toplevel_configure_bounds,
        .wm_capabilities = xdg_toplevel_wm_capabilities,
};

static void
output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
                int32_t physical_width, int32_t physical_height,
                int32_t subpixel, const char *make, const char *model,
                int32_t transform)
{
        // Información física del monitor. Útil para subpixel rendering.
}

static void
output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
            int32_t width, int32_t height, int32_t refresh)
{
        // refresh está en mHz (ej: 60000 para 60Hz).
        // Útil para sincronizar animaciones si no se usa wl_surface_frame.
}

static void
output_done(void *data, struct wl_output *wl_output)
{
        // "Commit" atómico de los eventos anteriores.
        // Aquí es donde se debería aplicar la lógica de redibujado si cambió la escala.
}

static void
output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
        // Este evento se recibe si el usuario tiene escalado activado (ej. 200% en GNOME).
        printf("Monitor scale factor detected: %d\n", factor);
        current_output_scale = factor;

        /* NOTA: Aquí se debería activar un flag para redimensionar el buffer
           en el siguiente frame (ancho * escala, alto * escala). */
}

/* Versión 4 añade name y description */
static void
output_name(void *data, struct wl_output *wl_output, const char *name)
{
}

static void
output_description(void *data, struct wl_output *wl_output, const char *description)
{
}

static const struct wl_output_listener output_listener = {
        .geometry = output_geometry,
        .mode = output_mode,
        .done = output_done,
        .scale = output_scale,
        .name = output_name,
        .description = output_description,
};


static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
        if (0) {
        } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
                compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
        } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                uint32_t version_to_bind = version < 6 ? version : 6;
                xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version_to_bind);
                xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
        } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
                wl_seat_add_listener(seat, &seat_listener, NULL);
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
                /* Vinculamos wl_output versión 3 para tener soporte de 'scale' */
                if (!global_output) {
                        global_output = wl_registry_bind(registry, name, &wl_output_interface, 3);
                        wl_output_add_listener(global_output, &output_listener, NULL);
                }
        }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
        .global = registry_handle_global,
        .global_remove = registry_handle_global_remove,
};

static void
surface_enter(void *data, struct wl_surface *wl_surface, struct wl_output *output)
{
        /* La ventana entró en un monitor.
           Aquí deberíamos consultar el 'scale' de ESE output específico
           y repintar la ventana con esa resolución. */
        // printf("Window entered output %p\n", (void*)output);
}

static void
surface_leave(void *data, struct wl_surface *wl_surface, struct wl_output *output)
{
        // La ventana salió de un monitor.
}

/* Listener para la superficie (versión moderna) */
static const struct wl_surface_listener surface_listener = {
        .enter = surface_enter,
        .leave = surface_leave,
        // .preferred_buffer_scale o .preferred_buffer_transform (Versión 6+)
};

void
wayland_set_title(char *title)
{
        if (current_title) free(current_title);
        current_title = strdup(title);
        xdg_toplevel_set_title(xdg_toplevel, current_title);
}

int
wayland_init(void)
{
        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!xkb_context) {
                printf("Error: Failed to create xkb_context\n");
                return 1;
        }

        display = wl_display_connect(NULL);
        if (!display) {
                printf("Error: Failed to connect to display\n");
                return 1;
        }

        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, NULL);
        wl_display_roundtrip(display);

        if (!compositor || !shm || !xdg_wm_base) {
                printf("Error: Wayland globals missing\n");
                return 1;
        }

        surface = wl_compositor_create_surface(compositor);
        wl_surface_add_listener(surface, &surface_listener, NULL); // <-- Añadir esto
        xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
        xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
        xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
        xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
        wayland_set_title(TITLE);
        wl_surface_commit(surface);

        if (fb_create(800, 600) != 0) {
                printf("Error! Can not create framebuffer\n");
                return 1;
        }

        wl_display_roundtrip(display);
        wayland_present();
        init = 1;
        return 0;
}

int
wayland_dispatch_events(void)
{
        if (wl_display_dispatch(display) < 0) {
                printf("wl_display_dispatch fails\n");
                return 1;
        }
        wl_display_flush(display);
        if (should_quit) {
                // printf("Close event!\n");
                return 1;
        }
        return 0;
}

void
wayland_present(void)
{
        if (should_quit || !surface) return;
        struct wl_buffer *buffer = fb_get_ready_buffer();
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, screen_fb.width, screen_fb.height);
        wl_surface_commit(surface);
        fb_swap();
}

void
wayland_cleanup(void)
{
        fb_destroy();
        if (xdg_toplevel) xdg_toplevel_destroy(xdg_toplevel);
        if (xdg_surface) xdg_surface_destroy(xdg_surface);
        if (surface) wl_surface_destroy(surface);
        if (xkb_state) xkb_state_unref(xkb_state);
        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_context) xkb_context_unref(xkb_context);
        if (display) wl_display_disconnect(display);
        if (current_title) free(current_title);
        should_quit = true;
}
