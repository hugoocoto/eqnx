#include <assert.h>
#include <stdio.h>

#include "../src/plug_api.h"
#include "theme.h"

int last_pressed_char = ' ';
Window *self_window;

void
resize(int x, int y, int w, int h)
{
}

void
kp_event(int sym, int mods)
{
        if (sym == ' ') {
                assert(fb_capture("screen_capture.png"));
                return;
        }
        last_pressed_char = sym;
        ask_for_redraw();
}

const char *
btn_repr(int b)
{
        const char *const lookup[] = {
                // clang-format off
[BTN_0] = "BTN_0", [BTN_1] = "BTN_1", [BTN_2] = "BTN_2", [BTN_3] = "BTN_3",
[BTN_4] = "BTN_4", [BTN_5] = "BTN_5", [BTN_6] = "BTN_6", [BTN_7] = "BTN_7",
[BTN_8] = "BTN_8", [BTN_9] = "BTN_9", [BTN_LEFT] = "BTN_LEFT", [BTN_RIGHT] =
        "BTN_RIGHT", [BTN_MIDDLE] = "BTN_MIDDLE", [BTN_SIDE] = "BTN_SIDE",
[BTN_EXTRA] = "BTN_EXTRA", [BTN_FORWARD] = "BTN_FORWARD", [BTN_BACK] =
        "BTN_BACK", [BTN_TASK] = "BTN_TASK", [BTN_JOYSTICK] = "BTN_TRIGGER",
[BTN_THUMB] = "BTN_THUMB", [BTN_THUMB2] = "BTN_THUMB2", [BTN_TOP] = "BTN_TOP",
[BTN_TOP2] = "BTN_TOP2", [BTN_PINKIE] = "BTN_PINKIE", [BTN_BASE] = "BTN_BASE",
[BTN_BASE2] = "BTN_BASE2", [BTN_BASE3] = "BTN_BASE3", [BTN_BASE4] = "BTN_BASE4",
[BTN_BASE5] = "BTN_BASE5", [BTN_BASE6] = "BTN_BASE6", [BTN_DEAD] = "BTN_DEAD",
[BTN_A] = "BTN_A", [BTN_B] = "BTN_B", [BTN_C] = "BTN_C", [BTN_X] = "BTN_X",
[BTN_Y] = "BTN_Y", [BTN_Z] = "BTN_Z", [BTN_TL] = "BTN_TL", [BTN_TR] = "BTN_TR",
[BTN_TL2] = "BTN_TL2", [BTN_TR2] = "BTN_TR2", [BTN_SELECT] = "BTN_SELECT",
[BTN_START] = "BTN_START", [BTN_MODE] = "BTN_MODE", [BTN_THUMBL] = "BTN_THUMBL",
[BTN_THUMBR] = "BTN_THUMBR", [BTN_DIGI] = "BTN_DIGI", [BTN_TOOL_RUBBER] =
        "BTN_TOOL_RUBBER", [BTN_TOOL_BRUSH] = "BTN_TOOL_BRUSH",
[BTN_TOOL_PENCIL] = "BTN_TOOL_PENCIL", [BTN_TOOL_AIRBRUSH] =
        "BTN_TOOL_AIRBRUSH", [BTN_TOOL_FINGER] = "BTN_TOOL_FINGER",
[BTN_TOOL_MOUSE] = "BTN_TOOL_MOUSE", [BTN_TOOL_LENS] = "BTN_TOOL_LENS",
[BTN_TOOL_QUINTTAP] = "BTN_TOOL_QUINTTAP", [BTN_STYLUS3] = "BTN_STYLUS3",
[BTN_TOUCH] = "BTN_TOUCH", [BTN_STYLUS] = "BTN_STYLUS", [BTN_STYLUS2] =
        "BTN_STYLUS2", [BTN_TOOL_DOUBLETAP] = "BTN_TOOL_DOUBLETAP",
[BTN_TOOL_TRIPLETAP] = "BTN_TOOL_TRIPLETAP", [BTN_TOOL_QUADTAP] =
        "BTN_TOOL_QUADTAP", [BTN_GEAR_DOWN] = "BTN_GEAR_DOWN", [BTN_GEAR_UP] =
                "BTN_GEAR_UP",
                // clang-format on
        };
        if (b >= 0 && b <= (int) (sizeof lookup / sizeof lookup[0])) {
                return lookup[b];
        }
        return NULL;
}

void
mouse_event(Pointer_Event e)
{
        uint32_t bg = RED;
        uint32_t fg = WHITE;
        switch (e.type) {
        case Pointer_Move:
                window_clear_line(self_window, 0, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 0, fg, bg,
                              "Pointer moves to: %d, %d (%d, %d)\n",
                              e.x, e.y, e.px, e.py);
                break;
        case Pointer_Enter:
                window_clear_line(self_window, 4, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 4, fg, bg,
                              "Pointer enter focus\n");
                break;
        case Pointer_Leave:
                window_clear_line(self_window, 4, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 4, fg, bg,
                              "Pointer leaves focus\n");
                break;
        case Pointer_Scroll:
                window_clear_line(self_window, 3, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 3, fg, bg,
                              "Pointer scrolls %d (on %d, %d (%d, %d))\n",
                              e.scroll, e.x, e.y, e.px, e.py);
                break;
        case Pointer_Scroll_Relative:
                window_clear_line(self_window, 3, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 3, fg, bg,
                              "Pointer scrolls relative axis=%d direction=%d (on %d, %d (%d, %d))\n",
                              e.axis, e.direction, e.x, e.y, e.px, e.py);
                break;
        case Pointer_Press:
                window_clear_line(self_window, 1, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 1, fg, bg,
                              "Pointer press %s (on %d, %d (%d, %d))\n",
                              btn_repr(e.btn), e.x, e.y, e.px, e.py);
                break;
        case Pointer_Release:
                window_clear_line(self_window, 1, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 1, fg, bg,
                              "Pointer release %s (on %d, %d (%d, %d))\n",
                              btn_repr(e.btn), e.x, e.y, e.px, e.py);
                break;
        default:
                window_clear_line(self_window, 1, BACKGROUND, BACKGROUND);
                window_printf(self_window, 0, 1, fg, bg,
                              "Unhandled mouse event %d\n", e.type);
                return;
        }
        ask_for_redraw();
}

void
render()
{
        // assert(my_window);
        // if (last_pressed_char) window_setall(my_window, last_pressed_char, GRAY, BLACK);
}

int
main(int argc, char **argv)
{
        assert(argc == 1);
        // printf("(Plugin: %s) Hello!\n", argv[0]);
        mainloop();
        // printf("(Plugin: %s) returns\n", argv[0]);
        return 0;
}
