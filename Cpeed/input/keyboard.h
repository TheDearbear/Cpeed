#pragma once

#include <stdint.h>

typedef enum CpdKeyCode {
    CpdKeyCode_Invalid,

    // Special

    CpdKeyCode_Backspace,
    CpdKeyCode_Tab,
    CpdKeyCode_Enter,
    CpdKeyCode_Shift,
    CpdKeyCode_Control,
    CpdKeyCode_Alt,
    CpdKeyCode_Escape,
    CpdKeyCode_Spacebar,
    CpdKeyCode_PageUp,
    CpdKeyCode_PageDown,
    CpdKeyCode_End,
    CpdKeyCode_Home,
    CpdKeyCode_Insert,
    CpdKeyCode_Delete,

    // Arrows

    CpdKeyCode_LeftArrow,
    CpdKeyCode_RightArrow,
    CpdKeyCode_UpArrow,
    CpdKeyCode_DownArrow,

    // Function keys

    CpdKeyCode_F1,
    CpdKeyCode_F2,
    CpdKeyCode_F3,
    CpdKeyCode_F4,
    CpdKeyCode_F5,
    CpdKeyCode_F6,
    CpdKeyCode_F7,
    CpdKeyCode_F8,
    CpdKeyCode_F9,
    CpdKeyCode_F10,
    CpdKeyCode_F11,
    CpdKeyCode_F12,
    CpdKeyCode_F13,
    CpdKeyCode_F14,
    CpdKeyCode_F15,
    CpdKeyCode_F16,
    CpdKeyCode_F17,
    CpdKeyCode_F18,
    CpdKeyCode_F19,
    CpdKeyCode_F20,
    CpdKeyCode_F21,
    CpdKeyCode_F22,
    CpdKeyCode_F23,
    CpdKeyCode_F24,

    // Alphabet

    CpdKeyCode_A,
    CpdKeyCode_B,
    CpdKeyCode_C,
    CpdKeyCode_D,
    CpdKeyCode_E,
    CpdKeyCode_F,
    CpdKeyCode_G,
    CpdKeyCode_H,
    CpdKeyCode_I,
    CpdKeyCode_J,
    CpdKeyCode_K,
    CpdKeyCode_L,
    CpdKeyCode_M,
    CpdKeyCode_N,
    CpdKeyCode_O,
    CpdKeyCode_P,
    CpdKeyCode_Q,
    CpdKeyCode_R,
    CpdKeyCode_S,
    CpdKeyCode_T,
    CpdKeyCode_U,
    CpdKeyCode_V,
    CpdKeyCode_W,
    CpdKeyCode_X,
    CpdKeyCode_Y,
    CpdKeyCode_Z,

    // Digits

    CpdKeyCode_0,
    CpdKeyCode_1,
    CpdKeyCode_2,
    CpdKeyCode_3,
    CpdKeyCode_4,
    CpdKeyCode_5,
    CpdKeyCode_6,
    CpdKeyCode_7,
    CpdKeyCode_8,
    CpdKeyCode_9,

    // Numpad Digits

    CpdKeyCode_Numpad0,
    CpdKeyCode_Numpad1,
    CpdKeyCode_Numpad2,
    CpdKeyCode_Numpad3,
    CpdKeyCode_Numpad4,
    CpdKeyCode_Numpad5,
    CpdKeyCode_Numpad6,
    CpdKeyCode_Numpad7,
    CpdKeyCode_Numpad8,
    CpdKeyCode_Numpad9,

    // Symbols

    CpdKeyCode_Multiply,
    CpdKeyCode_Minus,
    CpdKeyCode_Plus,

    CpdKeyCode_Equal,

    CpdKeyCode_Slash,
    CpdKeyCode_Backslash,

    CpdKeyCode_Dot,
    CpdKeyCode_Comma,
    CpdKeyCode_Semicolon,

    CpdKeyCode_LeftSquareBracket,
    CpdKeyCode_RightSquareBracket,

    CpdKeyCode_Quote,
    CpdKeyCode_Backtick
} CpdKeyCode;

typedef enum CpdInputMode {
    CpdInputMode_KeyCode,
    CpdInputMode_Text
} CpdInputMode;

typedef struct CpdButtonPressInputEventData {
    CpdKeyCode key_code;
    bool pressed;
} CpdButtonPressInputEventData;

typedef struct CpdCharInputEventData {
    // UTF-8 encoded char
    uint32_t character;
    // Length of char in bytes. Must be inside segment [1; 4]
    uint32_t length;
} CpdCharInputEventData;

typedef struct CpdTextInputEventData {
    // UTF-8 encoded text. You should stop using this pointer before next input query or window destroying
    char* text;
} CpdTextInputEventData;
