#include <malloc.h>

#include "uwpmain.h"

bool set_window_input_mode(CpdWindow window, CpdInputMode mode) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    if (uwp_window->input_queue_size != 0) {
        return false;
    }

    uwp_window->input_mode = mode;

    return true;
}

CpdInputMode get_window_input_mode(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    return uwp_window->input_mode;
}

void clear_window_event_queue(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    cleanup_input_queue(uwp_window->input_queue, uwp_window->input_queue_size);

    uwp_window->input_queue_size = 0;
}

bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    if (uwp_window->input_queue_size == 0) {
        return false;
    }

    if (uwp_window->resize_swap_queue) {
        CpdInputEvent* swap_queue = (CpdInputEvent*)malloc(uwp_window->input_queue_max_size * sizeof(CpdInputEvent));

        if (swap_queue == 0) {
            return false;
        }

        free(uwp_window->input_swap_queue);
        uwp_window->input_swap_queue = swap_queue;
        uwp_window->resize_swap_queue = false;
    }

    *events = uwp_window->input_queue;
    *size = uwp_window->input_queue_size;

    uint32_t temp_size = uwp_window->input_queue_size;
    uwp_window->input_queue_size = uwp_window->input_swap_queue_size;
    uwp_window->input_swap_queue_size = temp_size;

    CpdInputEvent* temp_queue = uwp_window->input_queue;
    uwp_window->input_queue = uwp_window->input_swap_queue;
    uwp_window->input_swap_queue = temp_queue;

    clear_window_event_queue(window);

    return true;
}

uint16_t get_gamepad_count(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;
    uint16_t count = 0;

    for (CpdInputDevice* current = uwp_window->devices; current != 0; current = current->next) {
        count++;
    }

    return count;
}

CpdGamepadStickPosition get_gamepad_stick_position(CpdWindow window, uint16_t gamepad_index, CpdGamepadStick stick) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;
    CpdInputDevice* current = uwp_window->devices;

    for (uint32_t i = 0; current != 0 && i < gamepad_index; i++) {
        current = current->next;
    }

    if (current == 0) {
        return (CpdGamepadStickPosition) {
            .x = (float)0xFFFFFFFFu,
            .y = (float)0xFFFFFFFFu
        };
    }

    if (stick == CpdGamepadStick_Left) {
        return (CpdGamepadStickPosition) {
            .x = (float)current->state.LeftThumbstickX,
            .y = (float)current->state.LeftThumbstickY
        };
    }

    return (CpdGamepadStickPosition) {
        .x = (float)current->state.RightThumbstickX,
        .y = (float)current->state.RightThumbstickY
    };
}

CpdGamepadTriggersPosition get_gamepad_triggers_position(CpdWindow window, uint16_t gamepad_index) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;
    CpdInputDevice* current = uwp_window->devices;

    for (uint32_t i = 0; current != 0 && i < gamepad_index; i++) {
        current = current->next;
    }

    if (current == 0) {
        return (CpdGamepadTriggersPosition) {
            .left = (float)0xFFFFFFFFu,
            .right = (float)0xFFFFFFFFu
        };
    }

    return (CpdGamepadTriggersPosition) {
        .left = (float)current->state.LeftTrigger,
        .right = (float)current->state.RightTrigger
    };
}
