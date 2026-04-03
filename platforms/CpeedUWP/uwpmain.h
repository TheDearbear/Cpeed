#pragma once

#define WIN32_LEAN_AND_MEAN
#define CINTERFACE

#include <malloc.h>
#include <string.h>

#include <windows.h>
#include <winstring.h>

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
extern void* g_main_core_window;

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

inline WCHAR* wide_string_from_utf8(const char* src, UINT* wide_len) {
    int str_length = (int)(strlen(src) + 1);
    int required_length = MultiByteToWideChar(CP_UTF8, 0, src, str_length, NULL, 0);

    WCHAR* wide_str = (WCHAR*)malloc(required_length * sizeof(WCHAR));
    if (wide_str == NULL) {
        return NULL;
    }

    MultiByteToWideChar(CP_UTF8, 0, src, str_length, wide_str, required_length);

    if (wide_len != NULL) {
        *wide_len = str_length;
    }

    return wide_str;
}

inline HSTRING create_hstring_from_utf8(const char* src) {
    int str_length = (int)strlen(src);
    int required_length = MultiByteToWideChar(CP_UTF8, 0, src, str_length, NULL, 0);

    WCHAR* wide_str = (WCHAR*)malloc(required_length * sizeof(WCHAR));
    if (wide_str == NULL) {
        return NULL;
    }

    MultiByteToWideChar(CP_UTF8, 0, src, str_length, wide_str, required_length);

    HSTRING hstr = NULL;
    WindowsCreateString(wide_str, required_length, &hstr);

    return hstr;
}
