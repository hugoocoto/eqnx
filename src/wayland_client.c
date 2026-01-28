#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Todo: refactor this vive-coded file.

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* Se requiere para la gestión del cursor - compilar con -lwayland-cursor */
#include <wayland-cursor.h>

#include "../wayland-protocol/xdg-decoration-unstable-v1.h"
#include "../wayland-protocol/xdg-shell-client-protocol.h"
#include "draw.h"
#include "event.h"
#include "keypress.h"
#include "wayland_client.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"

#define TITLE "Untitled"

/* Globales Wayland */
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
static struct wl_output *global_output = NULL;
static struct zxdg_decoration_manager_v1 *decoration_manager = NULL;

/* Gestión de Cursor */
static struct wl_cursor_theme *cursor_theme = NULL;
static struct wl_surface *cursor_surface = NULL;
static struct wl_cursor *default_cursor = NULL;

/* Estado */
static int32_t current_output_scale = 1;
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
static bool configured = false;

/* Control de flujo de cuadros (VSync) */
static struct wl_callback *frame_callback = NULL;
static bool waiting_for_frame = false;

struct Framebuffer {
        struct wl_buffer *buffers[2];
        bool busy[2];
        uint32_t *data;
        size_t capacity;
        int fd;
        int width, height;
        int logical_w, logical_h;
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
buffer_release(void *data, struct wl_buffer *buffer)
{
        struct Framebuffer *fb = data;
        if (buffer == fb->buffers[0]) fb->busy[0] = false;
        if (buffer == fb->buffers[1]) fb->busy[1] = false;
}

static const struct wl_buffer_listener buffer_listener = {
        .release = buffer_release,
};

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
        wl_buffer_add_listener(screen_fb.buffers[0], &buffer_listener, &screen_fb);
        screen_fb.busy[0] = false;

        screen_fb.buffers[1] = wl_shm_pool_create_buffer(pool, page_size, w, h, stride, WL_SHM_FORMAT_ARGB8888);
        wl_buffer_add_listener(screen_fb.buffers[1], &buffer_listener, &screen_fb);
        screen_fb.busy[1] = false;

        wl_shm_pool_destroy(pool);
        return 0;
}

static int
fb_create(int logical_w, int logical_h)
{
        int physical_w = logical_w * current_output_scale;
        int physical_h = logical_h * current_output_scale;

        screen_fb = (struct Framebuffer) {
                .width = physical_w,
                .height = physical_h,
                .logical_w = logical_w,
                .logical_h = logical_h,
                .stride = physical_w * 4,
                .fd = -1,
                .current_idx = 0
        };

        return init_buffers(physical_w, physical_h, screen_fb.stride);
}

static int
fb_resize(int logical_w, int logical_h)
{
        if (logical_w <= 0 || logical_h <= 0) return 1;

        int physical_w = logical_w * current_output_scale;
        int physical_h = logical_h * current_output_scale;

        if (physical_w == screen_fb.width && physical_h == screen_fb.height) return 0;

        screen_fb.logical_w = logical_w;
        screen_fb.logical_h = logical_h;
        screen_fb.width = physical_w;
        screen_fb.height = physical_h;
        screen_fb.stride = physical_w * 4;

        return init_buffers(physical_w, physical_h, screen_fb.stride);
}

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
        *w = screen_fb.logical_w;
        *h = screen_fb.logical_h;
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
        int next_idx = 1 - screen_fb.current_idx;
        if (!screen_fb.busy[next_idx]) {
                screen_fb.current_idx = next_idx;
        }
        fb_clear(0xFF000000);
}

struct wl_buffer *
fb_get_ready_buffer()
{
        return screen_fb.buffers[screen_fb.current_idx];
}

/* --- Cursor --- */
static void
update_cursor(uint32_t serial)
{
        if (!pointer || !cursor_theme) return;
        struct wl_cursor_image *image = default_cursor->images[0];
        struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer(image);
        wl_pointer_set_cursor(pointer, serial, cursor_surface, image->hotspot_x, image->hotspot_y);
        wl_surface_attach(cursor_surface, cursor_buffer, 0, 0);
        wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
        wl_surface_set_buffer_scale(cursor_surface, current_output_scale);
        wl_surface_commit(cursor_surface);
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
        .type = Pointer_Move, .px = pointer_x, .py = pointer_y, .x = pointer_x / get_grid_width(f), .y = pointer_y / f->l_h });
}

static void
pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
        update_cursor(serial);
        notify_pointer_event((Pointer_Event) { .type = Pointer_Enter });
}
static void
pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface)
{
        notify_pointer_event((Pointer_Event) { .type = Pointer_Leave });
}
static void
pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
        pending_pointer_x = wl_fixed_to_double(sx);
        pending_pointer_y = wl_fixed_to_double(sy);
        has_pending_pointer = true;
}
static void
pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = state == WL_POINTER_BUTTON_STATE_PRESSED ? Pointer_Press : Pointer_Release,
        .px = pointer_x,
        .py = pointer_y,
        .x = pointer_x / get_grid_width(f),
        .y = pointer_y / f->l_h,
        .btn = button });
}
static void
pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
        Font *f = get_default_font();
        notify_pointer_event((Pointer_Event) {
        .type = Pointer_Scroll, .px = pointer_x, .py = pointer_y, .x = pointer_x / get_grid_width(f), .y = pointer_y / f->l_h, .scroll = value });
}
static void
pointer_axis_source(void *d, struct wl_pointer *p, uint32_t as)
{
}
static void
pointer_axis_stop(void *d, struct wl_pointer *p, uint32_t t, uint32_t a)
{
}
static void
pointer_axis_discrete(void *d, struct wl_pointer *p, uint32_t a, int32_t ds)
{
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
        .axis_discrete = pointer_axis_discrete
};

/* --- Teclado --- */
static void
keyboard_keymap(void *data, struct wl_keyboard *k, uint32_t f, int32_t fd, uint32_t s)
{
        if (f != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
                close(fd);
                return;
        }
        char *map = mmap(NULL, s, PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
                close(fd);
                return;
        }
        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_state) xkb_state_unref(xkb_state);
        xkb_keymap = xkb_keymap_new_from_string(xkb_context, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(map, s);
        close(fd);
        if (xkb_keymap) xkb_state = xkb_state_new(xkb_keymap);
}
static void
keyboard_enter(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf, struct wl_array *keys)
{
}
static void
keyboard_leave(void *d, struct wl_keyboard *k, uint32_t s, struct wl_surface *surf)
{
}

/* CORREGIDO: Lógica original restaurada para ignorar keypress en modificadores */
static void
keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
        static int mods = 0;
        uint32_t keycode = key + 8;
        xkb_keysym_t sym = xkb_state_key_get_one_sym(xkb_state, keycode);

        if (sym == XKB_KEY_NoSymbol) return;

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

                /* Modificadores: Solo actualizan estado, NO envían evento */
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
keyboard_modifiers(void *d, struct wl_keyboard *k, uint32_t s, uint32_t md, uint32_t ml, uint32_t mlo, uint32_t g)
{
        if (xkb_state) xkb_state_update_mask(xkb_state, md, ml, mlo, 0, 0, g);
}
static void
keyboard_repeat_info(void *d, struct wl_keyboard *k, int32_t r, int32_t de)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
        .keymap = keyboard_keymap,
        .enter = keyboard_enter,
        .leave = keyboard_leave,
        .key = keyboard_key,
        .modifiers = keyboard_modifiers,
        .repeat_info = keyboard_repeat_info
};

/* --- Seat --- */
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
seat_name(void *d, struct wl_seat *s, const char *n)
{
}
static const struct wl_seat_listener seat_listener = { .capabilities = seat_capabilities, .name = seat_name };

/* --- XDG Shell --- */
static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
        xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = { .ping = xdg_wm_base_ping };

static void
xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
        xdg_surface_ack_configure(xdg_surface, serial);

        int w = pending_width;
        int h = pending_height;
        if (w == 0) w = 800; // Tamaño por defecto si no lo da el WM
        if (h == 0) h = 600;

        if (w != screen_fb.logical_w || h != screen_fb.logical_h) {
                fb_resize(w, h);
                notify_resize_event(w, h);
        }

        configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = { .configure = xdg_surface_configure };

static void
xdg_toplevel_configure(void *data, struct xdg_toplevel *t, int32_t w, int32_t h, struct wl_array *s)
{
        pending_width = w;
        pending_height = h;
}
static void
xdg_toplevel_close(void *d, struct xdg_toplevel *t)
{
        should_quit = true;
}
static void
xdg_toplevel_configure_bounds(void *d, struct xdg_toplevel *t, int32_t w, int32_t h)
{
}
static void
xdg_toplevel_wm_capabilities(void *d, struct xdg_toplevel *t, struct wl_array *c)
{
}
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        .configure = xdg_toplevel_configure,
        .close = xdg_toplevel_close,
        .configure_bounds = xdg_toplevel_configure_bounds,
        .wm_capabilities = xdg_toplevel_wm_capabilities
};

/* --- Output --- */
static void
output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
        if (current_output_scale != factor) {
                current_output_scale = factor;
                if (cursor_theme) {
                        wl_cursor_theme_destroy(cursor_theme);
                        cursor_theme = wl_cursor_theme_load(NULL, 24 * factor, shm);
                        if (cursor_theme) default_cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
                }
                fb_resize(screen_fb.logical_w, screen_fb.logical_h);
        }
}
static void
output_geometry(void *d, struct wl_output *o, int32_t x, int32_t y, int32_t pw, int32_t ph, int32_t s, const char *m, const char *mo, int32_t t)
{
}
static void
output_mode(void *d, struct wl_output *o, uint32_t f, int32_t w, int32_t h, int32_t r)
{
}
static void
output_done(void *d, struct wl_output *o)
{
}
static void
output_name(void *d, struct wl_output *o, const char *n)
{
}
static void
output_description(void *d, struct wl_output *o, const char *de)
{
}
static const struct wl_output_listener output_listener = {
        .geometry = output_geometry,
        .mode = output_mode,
        .done = output_done,
        .scale = output_scale,
        .name = output_name,
        .description = output_description
};

/* --- Registro --- */
static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
        if (strcmp(interface, wl_compositor_interface.name) == 0) {
                compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
        } else if (strcmp(interface, wl_shm_interface.name) == 0) {
                shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version < 6 ? version : 6);
                xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
        } else if (strcmp(interface, wl_seat_interface.name) == 0) {
                seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
                wl_seat_add_listener(seat, &seat_listener, NULL);
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
                if (!global_output) {
                        global_output = wl_registry_bind(registry, name, &wl_output_interface, 3);
                        wl_output_add_listener(global_output, &output_listener, NULL);
                }
        } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
                decoration_manager = wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1);
        }
}

static void
registry_handle_global_remove(void *d, struct wl_registry *r, uint32_t n)
{
}
static const struct wl_registry_listener registry_listener = { .global = registry_handle_global, .global_remove = registry_handle_global_remove };

static void redraw(void *data, struct wl_callback *callback, uint32_t time);
static const struct wl_callback_listener frame_listener = { .done = redraw };

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
        wl_callback_destroy(callback);
        waiting_for_frame = false;
}

static void
surface_enter(void *d, struct wl_surface *s, struct wl_output *o)
{
}
static void
surface_leave(void *d, struct wl_surface *s, struct wl_output *o)
{
}
static const struct wl_surface_listener surface_listener = { .enter = surface_enter, .leave = surface_leave };

void
wayland_set_title(char *title)
{
        if (current_title) free(current_title);
        current_title = strdup(title);
        if (xdg_toplevel) xdg_toplevel_set_title(xdg_toplevel, current_title);
}

void
wayland_present(void)
{
        if (should_quit || !surface || !configured) return;

        if (waiting_for_frame) return;

        struct wl_buffer *buffer = fb_get_ready_buffer();
        if (!buffer) return;

        wl_surface_set_buffer_scale(surface, current_output_scale);
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, screen_fb.logical_w, screen_fb.logical_h);

        struct wl_callback *cb = wl_surface_frame(surface);
        wl_callback_add_listener(cb, &frame_listener, NULL);
        waiting_for_frame = true;

        wl_surface_commit(surface);
        fb_swap();
}

int
wayland_init(void)
{
        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!xkb_context) return 1;

        display = wl_display_connect(NULL);
        if (!display) {
                fprintf(stderr, "Cannot connect to Wayland display\n");
                return 1;
        }

        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &registry_listener, NULL);
        wl_display_roundtrip(display);

        if (!compositor || !shm || !xdg_wm_base) {
                fprintf(stderr, "Missing Wayland globals\n");
                return 1;
        }

        cursor_theme = wl_cursor_theme_load(NULL, 24 * current_output_scale, shm);
        if (cursor_theme) {
                default_cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
                cursor_surface = wl_compositor_create_surface(compositor);
        }

        surface = wl_compositor_create_surface(compositor);
        wl_surface_add_listener(surface, &surface_listener, NULL);

        xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
        xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

        xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
        xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

        if (decoration_manager) {
                struct zxdg_toplevel_decoration_v1 *decoration =
                zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_manager, xdg_toplevel);

                /* Solicitamos explícitamente decoración del lado del servidor (SSD) */
                zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }

        wayland_set_title(TITLE);

        /* Inicialización Síncrona */
        wl_surface_commit(surface);

        while (!configured) {
                if (wl_display_dispatch(display) < 0) return 1;
        }

        if (fb_create(screen_fb.logical_w, screen_fb.logical_h) != 0) return 1;

        fb_clear(0xFF000000);
        wayland_present();

        wl_display_flush(display);

        init = 1;
        return 0;
}

int
wayland_dispatch_events(void)
{
        if (wl_display_dispatch(display) < 0) return 1;
        if (should_quit) return 1;
        return 0;
}

void
wayland_cleanup(void)
{
        if (frame_callback) wl_callback_destroy(frame_callback);
        fb_destroy();
        if (cursor_surface) wl_surface_destroy(cursor_surface);
        if (cursor_theme) wl_cursor_theme_destroy(cursor_theme);
        if (xdg_toplevel) xdg_toplevel_destroy(xdg_toplevel);
        if (xdg_surface) xdg_surface_destroy(xdg_surface);
        if (surface) wl_surface_destroy(surface);
        if (pointer) wl_pointer_release(pointer);
        if (keyboard) wl_keyboard_release(keyboard);
        if (seat) wl_seat_destroy(seat);
        if (xkb_state) xkb_state_unref(xkb_state);
        if (xkb_keymap) xkb_keymap_unref(xkb_keymap);
        if (xkb_context) xkb_context_unref(xkb_context);
        if (global_output) wl_output_destroy(global_output);
        if (xdg_wm_base) xdg_wm_base_destroy(xdg_wm_base);
        if (shm) wl_shm_destroy(shm);
        if (compositor) wl_compositor_destroy(compositor);
        if (decoration_manager) zxdg_decoration_manager_v1_destroy(decoration_manager);
        if (current_title) free(current_title);
        if (registry) wl_registry_destroy(registry);
        if (display) wl_display_disconnect(display);
}
