#pragma once

#include "input/clipboard.h"
#include "input/gamepad.h"
#include "input/keyboard.h"
#include "input/mouse.h"

typedef enum CpdInputModifierKeyFlags {
    CpdInputModifierKey_None = 0,
    CpdInputModifierKey_Shift = 1 << 0,
    CpdInputModifierKey_Control = 1 << 1,
    CpdInputModifierKey_Alt = 1 << 2
} CpdInputModifierKeyFlags;

typedef enum CpdInputEventType {
    CpdInputEventType_None,

    // Clipboard

    CpdInputEventType_Clipboard,

    // Mouse

    CpdInputEventType_MouseMove,
    CpdInputEventType_MouseScroll,
    CpdInputEventType_MouseButtonPress,

    // Keyboard

    CpdInputEventType_ButtonPress,
    CpdInputEventType_CharInput,
    CpdInputEventType_TextInput,

    // Gamepad

    CpdInputEventType_GamepadButtonPress,
    CpdInputEventType_GamepadStick,
    CpdInputEventType_GamepadTrigger,
    CpdInputEventType_GamepadConnect
} CpdInputEventType;

typedef union CpdInputEventData {
    // Clipboard

    CpdClipboardInputEventData clipboard;

    // Mouse

    CpdMouseMoveInputEventData mouse_move;
    CpdMouseScrollInputEventData mouse_scroll;
    CpdMouseButtonPressInputEventData mouse_button_press;

    // Keyboard

    CpdButtonPressInputEventData button_press;
    CpdCharInputEventData char_input;
    CpdTextInputEventData text_input;

    // Gamepad

    CpdGamepadButtonPressInputEventData gamepad_button_press;
    CpdGamepadStickInputEventData gamepad_stick;
    CpdGamepadTriggerInputEventData gamepad_trigger;
    CpdGamepadConnectInputEventData gamepad_connect;
} CpdInputEventData;

typedef struct CpdInputEvent {
    CpdInputEventType type;
    CpdInputModifierKeyFlags modifiers;
    uint64_t time;
    CpdInputEventData data;
} CpdInputEvent;
