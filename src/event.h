#ifndef EVENT_H_
#define EVENT_H_ 1

#include "keypress.h"
#include <stdbool.h>

typedef struct Event Event;
typedef struct Pointer_Event Pointer_Event;

// button codes here
#include <linux/input-event-codes.h>

struct Pointer_Event {
        enum {
                Pointer_Move,
                Pointer_Press,
                Pointer_Release,
                Pointer_Enter,
                Pointer_Leave,
                Pointer_Scroll,
                Pointer_Scroll_Relative,
        } type;
        union {
                int btn;
                int scroll;
                struct {
                        int axis;
                        int direction;
                };
        };
        struct {
                int x, y, px, py;
        };
};

struct Event {
        enum {
                EventKp,
                EventPointer,
        } code;
        union {
                Keypress kp;
                Pointer_Event pointer;
        };
};

bool event_is_kp(Event e);
int event_kp_get_sym(Event e);
int event_kp_get_mods(Event e);

typedef void (*Resize_Listener)(int, int);
void add_resize_listener(Resize_Listener rl);
void notify_resize_event(int w, int h);

typedef void (*Pointer_Listener)(Pointer_Event);
void add_pointer_listener(Pointer_Listener pl);
void notify_pointer_event(Pointer_Event);

bool event_kp_has_mod_Alt(Event e);
bool event_kp_has_mod_Control(Event e);
bool event_kp_has_mod_Hyper(Event e);
bool event_kp_has_mod_Meta(Event e);
bool event_kp_has_mod_Shift(Event e);
bool event_kp_has_mod_Super(Event e);

bool event_kp_has_mod_Alt_L(Event e);
bool event_kp_has_mod_Alt_R(Event e);
bool event_kp_has_mod_Caps_Lock(Event e);
bool event_kp_has_mod_Control_L(Event e);
bool event_kp_has_mod_Control_R(Event e);
bool event_kp_has_mod_Hyper_L(Event e);
bool event_kp_has_mod_Hyper_R(Event e);
bool event_kp_has_mod_Meta_L(Event e);
bool event_kp_has_mod_Meta_R(Event e);
bool event_kp_has_mod_Shift_L(Event e);
bool event_kp_has_mod_Shift_R(Event e);
bool event_kp_has_mod_Super_L(Event e);
bool event_kp_has_mod_Super_R(Event e);
bool event_kp_has_mod_Shift_Lock(Event e);

#endif
