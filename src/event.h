#ifndef EVENT_H_
#define EVENT_H_ 1

#include "keypress.h"
#include <stdbool.h>

typedef struct Event Event;

typedef struct Event {
        enum {
                EventKp,
        } code;
        union {
                Keypress kp;
        };
} Event;


bool event_is_kp(Event e);
char *event_kp_get_utf8(Event e);
int event_kp_get_sym(Event e);
int event_kp_get_mods(Event e);

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
