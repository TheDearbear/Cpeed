#pragma once

#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <malloc.h>

#include "../vulkan.h"
#include "../platform.h"

#include <windowsx.h>

#define INPUT_QUEUE_BASE_SIZE 16
#define INPUT_QUEUE_SIZE_STEP 16

typedef struct WindowExtraData {
    CpdInputEvent* input_queue;
    CpdInputEvent* input_swap_queue;
    uint32_t input_queue_size;
    uint32_t input_swap_queue_size;
    uint32_t input_queue_max_size;
    CpdInputMode input_mode;
    CpdInputModifierKeyFlags current_key_modifiers;
    int16_t mouse_x;
    int16_t mouse_y;

    uint32_t should_close : 1;
    uint32_t resized : 1;
    uint32_t minimized : 1;
    uint32_t resize_swap_queue : 1;
    uint32_t first_mouse_event : 1;
} WindowExtraData;
