#ifndef CLIENT_H
#define CLIENT_H 1

#include "keypress.h"

// 0 -> success
int wayland_init(); 
int wayland_dispatch_events();
void wayland_present();
void wayland_cleanup();
void wayland_set_title(char *title);


#endif
