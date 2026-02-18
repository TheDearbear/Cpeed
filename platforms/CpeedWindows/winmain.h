#pragma once

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS

#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOCOLOR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#ifdef CPD_IMGUI_AVAILABLE
#include <dcimgui.h>
#endif

#include <stdbool.h>
#include <windows.h>

#include <Cpeed/platform/window.h>
#include <Cpeed/input.h>

#define INPUT_QUEUE_BASE_SIZE 16
#define INPUT_QUEUE_SIZE_STEP 16

#define KEYBOARD_PRESSES_BASE_SIZE 16
#define KEYBOARD_PRESSES_SIZE_STEP 16

#define GET_EXTRA_DATA(hWnd) ((WindowExtraData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA))

extern LARGE_INTEGER g_counter_frequency;
extern ATOM g_window_class;

typedef struct CpdKeyboardKey {
    uint16_t scan_code;
    bool pressed;
} CpdKeyboardKey;

typedef struct WindowExtraData {
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

    CpdKeyboardKey* keyboard_presses;
    uint32_t keyboard_presses_size;
    uint32_t keyboard_presses_max_size;

    struct CpdFrameLayer* layers;

#ifdef CPD_IMGUI_AVAILABLE
    ImGuiContext* imgui_context;
#endif

    uint32_t should_close : 1;
    uint32_t resized : 1;
    uint32_t minimized : 1;
    uint32_t resize_swap_queue : 1;
    uint32_t first_mouse_event : 1;
    uint32_t focused : 1;
    uint32_t use_raw_input : 1;

    uint32_t left_shift_pressed : 1;
    uint32_t right_shift_pressed : 1;
    uint32_t left_control_pressed : 1;
    uint32_t right_control_pressed : 1;
    uint32_t left_alt_pressed : 1;
    uint32_t right_alt_pressed : 1;
} WindowExtraData;

void cleanup_input_queue(CpdInputEvent* queue, uint32_t size);
bool resize_input_queue_if_need(WindowExtraData* data, uint32_t new_events);
bool set_keyboard_key_press(WindowExtraData* data, uint16_t key_code, bool pressed);
bool register_raw_input(HWND hWnd);
