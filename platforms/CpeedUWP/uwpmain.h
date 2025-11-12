#pragma once

#define WIN32_LEAN_AND_MEAN
#define CINTERFACE

#include <windows.h>

// HACK: Ignore header file 'Windows.Foundation.Collections.h'
// to make project compatible with old Windows SDKs that
// do not provide support for C
#define WINDOWS_FOUNDATION_COLLECTIONS_H

#include <Windows.Gaming.Input.h>

#include <Cpeed/common/math.h>
#include <Cpeed/input/keyboard.h>
#include <Cpeed/backend.h>

#define INPUT_QUEUE_BASE_SIZE 16
#define INPUT_QUEUE_SIZE_STEP 16

extern LARGE_INTEGER g_counter_frequency;

typedef struct CpdInputDevice {
    struct CpdInputDevice* next;
    __x_ABI_CWindows_CGaming_CInput_CGamepadReading state;
} CpdInputDevice;

typedef struct CpdUWPWindow {
    void* core_window;
    CpdBackendHandle backend;

    CpdInputEvent* input_queue;
    CpdInputEvent* input_swap_queue;
    uint32_t input_queue_size;
    uint32_t input_swap_queue_size;
    uint32_t input_queue_max_size;
    CpdInputMode input_mode;
    CpdInputModifierKeyFlags current_key_modifiers;
    CpdSize size;

    int32_t mouse_x;
    int32_t mouse_y;

    CpdInputDevice* devices;

    struct CpdFrameLayer* layers;

#ifdef CPD_IMGUI_AVAILABLE
    ImGuiContext* imgui_context;
#endif

    uint32_t should_close : 1;
    uint32_t resized : 1;
    uint32_t visible : 1;
    uint32_t resize_swap_queue : 1;
    uint32_t first_mouse_event : 1;
} CpdUWPWindow;

void cleanup_input_queue(CpdInputEvent* queue, uint32_t size);
bool resize_input_queue_if_need(CpdUWPWindow* uwp_window, uint32_t new_events);

void poll_events(CpdUWPWindow* uwp_window);
void poll_gamepads(CpdUWPWindow* uwp_window);

bool get_lowest_frame_layer(void* context, struct CpdFrameLayer* frame_layer);

bool add_button_press_to_queue(CpdUWPWindow* uwp_window, CpdKeyCode keyCode, bool pressed);
bool add_char_input_to_queue(CpdUWPWindow* uwp_window, uint32_t character, uint32_t length);
bool add_mouse_button_press_to_queue(CpdUWPWindow* uwp_window, CpdMouseButtonType button, bool pressed);
bool add_mouse_move_to_queue(CpdUWPWindow* uwp_window, int16_t x_pos, int16_t y_pos);
bool add_mouse_scroll_to_queue(CpdUWPWindow* uwp_window, int32_t vertical, int32_t horizontal);
bool add_gamepad_connect_to_queue(CpdUWPWindow* uwp_window, CpdGamepadConnectStatus status, uint16_t index);
bool add_gamepad_stick_to_queue(CpdUWPWindow* uwp_window, CpdGamepadStick stick, uint16_t index);
bool add_gamepad_trigger_to_queue(CpdUWPWindow* uwp_window, CpdGamepadTrigger trigger, uint16_t index);
bool add_gamepad_button_press_to_queue(CpdUWPWindow* uwp_window, CpdGamepadButtonType button, uint16_t index, bool pressed);
