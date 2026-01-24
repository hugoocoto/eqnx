#include <stdio.h>
#include <string.h>

#include "keypress.h"

Keypress_listener listener = NULL;

void
keypress_add_listener(Keypress_listener l)
{
        if (listener) // send something like a listener-disconnect event
                listener((Keypress) { .mods = 0, .sym = EOF });

        listener = l;
}

void
register_keypress(xkb_keysym_t sym, enum Key_mods mods, char repr[5])
{
        Keypress kp = (Keypress) {
                .sym = sym,
                .mods = mods,
                .utf8 = { 0 },
        };
        if (repr && *repr) snprintf(kp.utf8, sizeof(kp.utf8) - 1, "%s", repr);
        if (listener) listener(kp);
}

char *
mod_repr(enum Key_mods mods, char *buf, int size)
{
        if (size <= 0 || !buf) return NULL;
        *buf = 0;
        if (Mod_Shift_L & mods) strncat(buf, " Shift_L", size);
        if (Mod_Shift_R & mods) strncat(buf, " Shift_R", size);
        if (Mod_Control_L & mods) strncat(buf, " Control_L", size);
        if (Mod_Control_R & mods) strncat(buf, " Control_R", size);
        if (Mod_Caps_Lock & mods) strncat(buf, " Caps_Lock", size);
        if (Mod_Shift_Lock & mods) strncat(buf, " Shift_Lock", size);
        if (Mod_Meta_L & mods) strncat(buf, " Meta_L", size);
        if (Mod_Meta_R & mods) strncat(buf, " Meta_R", size);
        if (Mod_Alt_L & mods) strncat(buf, " Alt_L", size);
        if (Mod_Alt_R & mods) strncat(buf, " Alt_R", size);
        if (Mod_Super_L & mods) strncat(buf, " Super_L", size);
        if (Mod_Super_R & mods) strncat(buf, " Super_R", size);
        if (Mod_Hyper_L & mods) strncat(buf, " Hyper_L", size);
        if (Mod_Hyper_R & mods) strncat(buf, " Hyper_R", size);
        return buf;
}

void
debug_keypress_print(Keypress kp)
{
        char buf[24];
        char buf2[1024];
        int size;
        size = xkb_keysym_to_utf8(kp.sym, buf, sizeof buf);
        if (size > 0 && kp.mods) {
                printf("Keypress: %s with mods %s (%s)\n",
                       buf, mod_repr(kp.mods, buf2, sizeof buf2), kp.utf8);
        } else if (size > 0) {
                printf("Keypress: %s (%s)\n", buf, kp.utf8);
        } else if (kp.mods) {
                printf("Keypress: %x with mods %s (%s)\n",
                       kp.sym, mod_repr(kp.mods, buf2, sizeof buf2), kp.utf8);
        } else {
                printf("Keypress: %x (%s)\n", kp.sym, kp.utf8);
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
