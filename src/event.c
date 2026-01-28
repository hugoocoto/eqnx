#include "event.h"

static Resize_Listener resize_listener;

void
add_resize_listener(Resize_Listener rl)
{
        resize_listener = rl;
}

void
notify_resize_event(int x, int y, int w, int h)
{
        if (resize_listener) resize_listener(x, y, w, h);
}

static Pointer_Listener pointer_listener;

void
add_pointer_listener(Pointer_Listener pl)
{
        pointer_listener = pl;
}

void
notify_pointer_event(Pointer_Event e)
{
        if (pointer_listener) pointer_listener(e);
}

bool
event_is_kp(Event e)
{
        return e.code == EventKp;
}

int
event_kp_get_sym(Event e)
{
        int s = e.kp.sym;
        return e.code == EventKp ? s : XKB_KEY_NoSymbol;
}

int
event_kp_get_mods(Event e)
{
        int s = e.kp.mods;
        return e.code == EventKp ? s : 0;
}

// clang-format off
bool event_kp_has_mod_Alt(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Alt : 0; }
bool event_kp_has_mod_Alt_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Alt_L : 0; }
bool event_kp_has_mod_Alt_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Alt_R : 0; }
bool event_kp_has_mod_Caps_Lock(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Caps_Lock : 0; }
bool event_kp_has_mod_Control(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Control : 0; }
bool event_kp_has_mod_Control_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Control_L : 0; }
bool event_kp_has_mod_Control_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Control_R : 0; }
bool event_kp_has_mod_Hyper(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Hyper : 0; }
bool event_kp_has_mod_Hyper_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Hyper_L : 0; }
bool event_kp_has_mod_Hyper_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Hyper_R : 0; }
bool event_kp_has_mod_Meta(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Meta : 0; }
bool event_kp_has_mod_Meta_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Meta_L : 0; }
bool event_kp_has_mod_Meta_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Meta_R : 0; }
bool event_kp_has_mod_Shift(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Shift : 0; }
bool event_kp_has_mod_Shift_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Shift_L : 0; }
bool event_kp_has_mod_Shift_Lock(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Shift_Lock : 0; }
bool event_kp_has_mod_Shift_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Shift_R : 0; }
bool event_kp_has_mod_Super(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Super : 0; }
bool event_kp_has_mod_Super_L(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Super_L : 0; }
bool event_kp_has_mod_Super_R(Event e){ return e.code == EventKp ? e.kp.mods & Mod_Super_R : 0; }
// clang-format on
