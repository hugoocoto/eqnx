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
        if (repr && *repr) snprintf(kp.utf8, sizeof(kp.utf8), "%s", repr);
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
