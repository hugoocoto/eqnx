#include <stdio.h>
#include <string.h>

#include "keypress.h"

Keypress_listener listener = NULL;

void
add_keypress_listener(Keypress_listener l)
{
        if (listener) { // send something like a listener-disconnect event
                listener((Keypress) { .mods = 0, .sym = EOF });
        }
        listener = l;
}

void
register_keypress(xkb_keysym_t sym, enum Key_mods mods, uint32_t codepoint)
{
        if (listener) {
                listener((Keypress) {
                .sym = sym,
                .mods = mods,
                .codepoint = codepoint,
                });
        }
}

// clang-format off
bool kp_has_mod_Alt(Keypress kp){ return kp.mods & Mod_Alt; }
bool kp_has_mod_Alt_L(Keypress kp){ return kp.mods & Mod_Alt_L; }
bool kp_has_mod_Alt_R(Keypress kp){ return kp.mods & Mod_Alt_R; }
bool kp_has_mod_Caps_Lock(Keypress kp){ return kp.mods & Mod_Caps_Lock; }
bool kp_has_mod_Control(Keypress kp){ return kp.mods & Mod_Control; }
bool kp_has_mod_Control_L(Keypress kp){ return kp.mods & Mod_Control_L; }
bool kp_has_mod_Control_R(Keypress kp){ return kp.mods & Mod_Control_R; }
bool kp_has_mod_Hyper(Keypress kp){ return kp.mods & Mod_Hyper; }
bool kp_has_mod_Hyper_L(Keypress kp){ return kp.mods & Mod_Hyper_L; }
bool kp_has_mod_Hyper_R(Keypress kp){ return kp.mods & Mod_Hyper_R; }
bool kp_has_mod_Meta(Keypress kp){ return kp.mods & Mod_Meta; }
bool kp_has_mod_Meta_L(Keypress kp){ return kp.mods & Mod_Meta_L; }
bool kp_has_mod_Meta_R(Keypress kp){ return kp.mods & Mod_Meta_R; }
bool kp_has_mod_Shift(Keypress kp){ return kp.mods & Mod_Shift; }
bool kp_has_mod_Shift_L(Keypress kp){ return kp.mods & Mod_Shift_L; }
bool kp_has_mod_Shift_Lock(Keypress kp){ return kp.mods & Mod_Shift_Lock; }
bool kp_has_mod_Shift_R(Keypress kp){ return kp.mods & Mod_Shift_R; }
bool kp_has_mod_Super(Keypress kp){ return kp.mods & Mod_Super; }
bool kp_has_mod_Super_L(Keypress kp){ return kp.mods & Mod_Super_L; }
bool kp_has_mod_Super_R(Keypress kp){ return kp.mods & Mod_Super_R; }
// clang-format on

// clang-format off
bool mod_has_Alt(int mods){ return mods & Mod_Alt; }
bool mod_has_Alt_L(int mods){ return mods & Mod_Alt_L; }
bool mod_has_Alt_R(int mods){ return mods & Mod_Alt_R; }
bool mod_has_Caps_Lock(int mods){ return mods & Mod_Caps_Lock; }
bool mod_has_Control(int mods){ return mods & Mod_Control; }
bool mod_has_Control_L(int mods){ return mods & Mod_Control_L; }
bool mod_has_Control_R(int mods){ return mods & Mod_Control_R; }
bool mod_has_Hyper(int mods){ return mods & Mod_Hyper; }
bool mod_has_Hyper_L(int mods){ return mods & Mod_Hyper_L; }
bool mod_has_Hyper_R(int mods){ return mods & Mod_Hyper_R; }
bool mod_has_Meta(int mods){ return mods & Mod_Meta; }
bool mod_has_Meta_L(int mods){ return mods & Mod_Meta_L; }
bool mod_has_Meta_R(int mods){ return mods & Mod_Meta_R; }
bool mod_has_Shift(int mods){ return mods & Mod_Shift; }
bool mod_has_Shift_L(int mods){ return mods & Mod_Shift_L; }
bool mod_has_Shift_Lock(int mods){ return mods & Mod_Shift_Lock; }
bool mod_has_Shift_R(int mods){ return mods & Mod_Shift_R; }
bool mod_has_Super(int mods){ return mods & Mod_Super; }
bool mod_has_Super_L(int mods){ return mods & Mod_Super_L; }
bool mod_has_Super_R(int mods){ return mods & Mod_Super_R; }
// clang-format on
