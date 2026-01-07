#include "../../platform/input/gamepad.h"
#include "../../platform/logging.h"
#include "test_layer.h"

#ifdef CPD_IMGUI_AVAILABLE
#include <dcimgui.h>
#endif

static float brightness_step = 0.05f;

static bool input(void* context, CpdWindow window, CpdFrame* frame, const CpdInputEvent* event) {
    if (event->type == CpdInputEventType_CharInput) {
        uint64_t text = event->data.char_input.character;
        log_debug("Char input: %s (%d bytes)\n", (char*)&text, event->data.char_input.length);
    }
    else if (event->type == CpdInputEventType_ButtonPress) {
        if (event->data.button_press.pressed) {
            if (event->data.button_press.key_code == CpdKeyCode_Numpad7 && frame->background.x < 1.0f) {
                frame->background.x += brightness_step;
                log_debug("New red brightness: %.2f\n", frame->background.x);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Numpad4 && frame->background.x > 0.0f) {
                frame->background.x -= brightness_step;
                log_debug("New red brightness: %.2f\n", frame->background.x);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Numpad8 && frame->background.y < 1.0f) {
                frame->background.y += brightness_step;
                log_debug("New green brightness: %.2f\n", frame->background.y);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Numpad5 && frame->background.y > 0.0f) {
                frame->background.y -= brightness_step;
                log_debug("New green brightness: %.2f\n", frame->background.y);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Numpad9 && frame->background.z < 1.0f) {
                frame->background.z += brightness_step;
                log_debug("New blue brightness: %.2f\n", frame->background.z);
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Numpad6 && frame->background.z > 0.0f) {
                frame->background.z -= brightness_step;
                log_debug("New blue brightness: %.2f\n", frame->background.z);
            }
        }
        else if (event->data.button_press.key_code == CpdKeyCode_Escape) {
            close_window(window);
        }
    }
    else if (event->type == CpdInputEventType_Clipboard) {
        if (event->data.clipboard.action_type == CpdClipboardActionType_Paste) {
            log_debug("Paste data\n");
        }
        else if (event->data.clipboard.action_type == CpdClipboardActionType_Copy) {
            log_debug("Copy data\n");
        }
        else if (event->data.clipboard.action_type == CpdClipboardActionType_Cut) {
            log_debug("Cut data\n");
        }
    }
    else if (event->type == CpdInputEventType_GamepadConnect) {
        log_debug("[Gamepad %d] Connected=%d\n", event->data.gamepad_connect.gamepad_index, event->data.gamepad_connect.status);
    }
    else if (event->type == CpdInputEventType_GamepadStick && event->data.gamepad_stick.stick == CpdGamepadStick_Right) {
        CpdGamepadStickPosition pos = get_gamepad_stick_position(window, event->data.gamepad_stick.gamepad_index, event->data.gamepad_stick.stick);

        frame->background.x = pos.x < 0 ? -pos.x : pos.x;
        frame->background.y = pos.y < 0 ? -pos.y : pos.y;
    }
    else if (event->type == CpdInputEventType_GamepadTrigger && event->data.gamepad_trigger.trigger == CpdGamepadTrigger_Left) {
        CpdGamepadTriggersPosition pos = get_gamepad_triggers_position(window, event->data.gamepad_trigger.gamepad_index);

        frame->background.z = pos.left;
    }
    else {
        return false;
    }

    return true;
}

static void render(void* context, CpdFrame* frame) {
    //
}

#ifdef CPD_IMGUI_AVAILABLE
static void imgui(void* context) {
    ImGui_ShowDemoWindow(0);
}
#endif

CpdFrameLayerFlags g_frame_layer_flags_test = CpdFrameLayerFlags_None;
CpdFrameLayerFunctions g_frame_layer_functions_test = {
#ifdef CPD_IMGUI_AVAILABLE
    .imgui = imgui,
#endif
    .input = input,
    .render = render,
    .resize = 0,
    .first_frame = 0,
    .added = 0,
    .remove = 0
};
