#ifndef KEYPRESS_H_
#define KEYPRESS_H_ 1

#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

enum Key_mods {
        Mod_None = 0,
        Mod_Shift_L = 0x1 << 0,
        Mod_Shift_R = 0x1 << 1,
        Mod_Shift = Mod_Shift_L | Mod_Shift_R,
        Mod_Control_L = 0x1 << 2,
        Mod_Control_R = 0x1 << 3,
        Mod_Control = Mod_Control_L | Mod_Control_R,
        Mod_Caps_Lock = 0x1 << 4,
        Mod_Shift_Lock = 0x1 << 5,
        Mod_Meta_L = 0x1 << 6,
        Mod_Meta_R = 0x1 << 7,
        Mod_Meta = Mod_Meta_L | Mod_Meta_R,
        Mod_Alt_L = 0x1 << 8,
        Mod_Alt_R = 0x1 << 9,
        Mod_Alt = Mod_Alt_L | Mod_Alt_R,
        Mod_Super_L = 0x1 << 10,
        Mod_Super_R = 0x1 << 11,
        Mod_Super = Mod_Super_L | Mod_Super_R,
        Mod_Hyper_L = 0x1 << 12,
        Mod_Hyper_R = 0x1 << 13,
        Mod_Hyper = Mod_Hyper_L | Mod_Hyper_R,
};

// key symbols are defined here
#include <xkbcommon/xkbcommon-keysyms.h>

typedef struct {
        enum Key_mods mods;
        xkb_keysym_t sym;
        uint32_t codepoint;
} Keypress;

typedef void (*Keypress_listener)(Keypress);

void register_keypress(xkb_keysym_t sym, enum Key_mods mods, uint32_t codepoint);
void keypress_add_listener(Keypress_listener);

bool kp_has_mod_Alt(Keypress kp);
bool kp_has_mod_Alt_L(Keypress kp);
bool kp_has_mod_Alt_R(Keypress kp);
bool kp_has_mod_Caps_Lock(Keypress kp);
bool kp_has_mod_Control(Keypress kp);
bool kp_has_mod_Control_L(Keypress kp);
bool kp_has_mod_Control_R(Keypress kp);
bool kp_has_mod_Hyper(Keypress kp);
bool kp_has_mod_Hyper_L(Keypress kp);
bool kp_has_mod_Hyper_R(Keypress kp);
bool kp_has_mod_Meta(Keypress kp);
bool kp_has_mod_Meta_L(Keypress kp);
bool kp_has_mod_Meta_R(Keypress kp);
bool kp_has_mod_Shift(Keypress kp);
bool kp_has_mod_Shift_L(Keypress kp);
bool kp_has_mod_Shift_Lock(Keypress kp);
bool kp_has_mod_Shift_R(Keypress kp);
bool kp_has_mod_Super(Keypress kp);
bool kp_has_mod_Super_L(Keypress kp);
bool kp_has_mod_Super_R(Keypress kp);

bool mod_has_Alt(int mods);
bool mod_has_Alt_L(int mods);
bool mod_has_Alt_R(int mods);
bool mod_has_Caps_Lock(int mods);
bool mod_has_Control(int mods);
bool mod_has_Control_L(int mods);
bool mod_has_Control_R(int mods);
bool mod_has_Hyper(int mods);
bool mod_has_Hyper_L(int mods);
bool mod_has_Hyper_R(int mods);
bool mod_has_Meta(int mods);
bool mod_has_Meta_L(int mods);
bool mod_has_Meta_R(int mods);
bool mod_has_Shift(int mods);
bool mod_has_Shift_L(int mods);
bool mod_has_Shift_Lock(int mods);
bool mod_has_Shift_R(int mods);
bool mod_has_Super(int mods);
bool mod_has_Super_L(int mods);
bool mod_has_Super_R(int mods);


#endif
