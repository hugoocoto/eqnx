#ifndef CLIENT_H
#define CLIENT_H 1

#include "keypress.h"

// 0 -> success
int wayland_init();
int wayland_dispatch_events();
void wayland_present();
void wayland_cleanup();
void wayland_set_title(char *title);


/* Framebuffer things */
int fb_clear(uint32_t color);
void fb_get_size(int *w, int *h);
uint32_t *fb_get_active_data();
uint32_t *fb_get_unactive_data(); // just because (use fb_get_active_data instead)

int fb_capture(char *filename);
int wayland_get_fd();

void wayland_cancel_read();
int wayland_dispatch_pending();
int wayland_flush();
int wayland_prepare_read();
int wayland_read_events();
int wayland_should_close();

#endif
