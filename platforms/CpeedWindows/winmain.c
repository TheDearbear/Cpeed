#include <Cpeed/platform.h>

#include "winmain.h"

#include <hidusage.h>

IGameInput* g_game_input;
CpdGamepad* g_gamepads;
LARGE_INTEGER g_counter_frequency;
ATOM g_window_class;

CpdCompilePlatform compile_platform() {
    return CpdCompilePlatform_Windows;
}

const char* compile_platform_name() {
    return "Windows";
}

CpdPlatformBackendFlags platform_supported_backends() {
    return CpdPlatformBackendFlags_Vulkan;
}

uint64_t get_clock_usec() {
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    return counter.QuadPart * 1000000 / g_counter_frequency.QuadPart;
}

void cleanup_input_queue(CpdInputEvent* queue, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        CpdInputEvent* event = &queue[i];

        if (event->type == CpdInputEventType_TextInput) {
            free(event->data.text_input.text);
            event->data.text_input.text = 0;
        }
    }
}

bool resize_input_queue_if_need(WindowExtraData* data, uint32_t new_events) {
    uint32_t new_size = data->input_queue_size + new_events;

    if (data->input_queue_max_size >= new_size) {
        return true;
    }

    uint32_t new_max_size = new_size;
    uint32_t remainder = new_max_size % INPUT_QUEUE_SIZE_STEP;

    if (remainder != 0) {
        new_max_size += INPUT_QUEUE_SIZE_STEP - remainder;
    }

    CpdInputEvent* new_input_queue = (CpdInputEvent*)malloc(new_max_size * sizeof(CpdInputEvent));
    if (new_input_queue == 0) {
        return false;
    }

    for (uint32_t i = 0; i < data->input_queue_size; i++) {
        new_input_queue[i] = data->input_queue[i];
    }

    CpdInputEvent* old_input_queue = data->input_queue;

    data->input_queue = new_input_queue;
    data->input_queue_max_size = new_max_size;
    data->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}

bool set_keyboard_key_press(WindowExtraData* data, uint16_t scan_code, bool pressed) {
    CpdKeyboardKey* keyboard_presses = data->keyboard_presses;

    if (!pressed) {
        bool last_pressed_found = false;
        uint32_t last_pressed_index = 0;
        for (uint32_t i = 0; i < data->keyboard_presses_size; i++) {
            if (keyboard_presses[i].scan_code == scan_code) {
                keyboard_presses[i].pressed = false;

                if (i == data->keyboard_presses_size - 1) {
                    data->keyboard_presses_size = last_pressed_found ? last_pressed_index + 1 : 0;
                }

                break;
            }

            if (keyboard_presses[i].pressed) {
                last_pressed_index = i;
                last_pressed_found = true;
            }
        }

        return true;
    }

    for (uint32_t i = 0; i < data->keyboard_presses_size; i++) {
        if (keyboard_presses[i].scan_code == scan_code) {
            keyboard_presses[i] = (CpdKeyboardKey) {
                .scan_code = scan_code,
                .pressed = true
            };

            return true;
        }
    }

    for (uint32_t i = 0; i < data->keyboard_presses_size; i++) {
        if (!keyboard_presses[i].pressed) {
            keyboard_presses[i] = (CpdKeyboardKey) {
                .scan_code = scan_code,
                .pressed = true
            };

            return true;
        }
    }

    if (data->keyboard_presses_size >= data->keyboard_presses_max_size) {
        uint32_t new_max_size = data->keyboard_presses_size + 1;
        uint32_t remainder = new_max_size % KEYBOARD_PRESSES_SIZE_STEP;

        if (remainder != 0) {
            new_max_size += KEYBOARD_PRESSES_SIZE_STEP - remainder;
        }

        CpdKeyboardKey* new_keyboard_presses = (CpdKeyboardKey*)malloc(new_max_size * sizeof(CpdKeyboardKey));
        if (new_keyboard_presses == 0) {
            return false;
        }

        for (uint32_t i = 0; i < data->keyboard_presses_size; i++) {
            new_keyboard_presses[i] = data->keyboard_presses[i];
        }

        free(data->keyboard_presses);
        data->keyboard_presses = new_keyboard_presses;
        data->keyboard_presses_max_size = new_max_size;
    }

    data->keyboard_presses[data->keyboard_presses_size++] = (CpdKeyboardKey) {
        .scan_code = scan_code,
        .pressed = pressed
    };

    return true;
}

bool register_raw_input(HWND hWnd) {
    RAWINPUTDEVICE device = {
        .usUsagePage = HID_USAGE_PAGE_GENERIC,
        .usUsage = HID_USAGE_GENERIC_KEYBOARD,
        .dwFlags = 0,
        .hwndTarget = hWnd
    };

    return RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE)) != FALSE;
}
