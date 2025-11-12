#include <malloc.h>

#include <Cpeed/platform/window.h>

#include "linuxevent.h"
#include "linuxmain.h"
#include "linuxwayland.h"

bool set_window_input_mode(CpdWindow window, CpdInputMode mode) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    wl_window->input_mode = mode;
    return true;
}

CpdInputMode get_window_input_mode(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return wl_window->input_mode;
}

void clear_window_event_queue(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    cleanup_input_queue(wl_window->input_queue, wl_window->input_queue_size);

    wl_window->input_queue_size = 0;
}

bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    insert_repeating_key_events(wl_window);

    if (wl_window->input_queue_size == 0) {
        return false;
    }

    if (wl_window->resize_swap_queue) {
        CpdInputEvent* swap_queue = (CpdInputEvent*)malloc(wl_window->input_queue_max_size * sizeof(CpdInputEvent));

        if (swap_queue == 0) {
            return false;
        }

        free(wl_window->input_swap_queue);
        wl_window->input_swap_queue = swap_queue;
        wl_window->resize_swap_queue = false;
    }

    *events = wl_window->input_queue;
    *size = wl_window->input_queue_size;

    uint32_t temp_size = wl_window->input_queue_size;
    wl_window->input_queue_size = wl_window->input_swap_queue_size;
    wl_window->input_swap_queue_size = temp_size;

    CpdInputEvent* temp_queue = wl_window->input_queue;
    wl_window->input_queue = wl_window->input_swap_queue;
    wl_window->input_swap_queue = temp_queue;

    clear_window_event_queue(window);

    return true;
}

uint16_t get_gamepad_count(CpdWindow window) {
    uint16_t count = 0;
    for (CpdEventDevice* current = g_event_devices; current != 0; current = current->next) {
        count++;
    }

    return count;
}

CpdGamepadStickPosition get_gamepad_stick_position(CpdWindow window, uint16_t gamepad_index, CpdGamepadStick stick) {
    CpdEventDevice* device = g_event_devices;

    for (uint16_t i = 0; i < gamepad_index; i++) {
        device = device->next;
    }

    if (stick == CpdGamepadStick_Left) {
        return (CpdGamepadStickPosition) {
            .x = (float)(uint16_t)(device->left_stick_x + INT16_MIN) * 2 / UINT16_MAX - 1.0f,
            .y = (float)(uint16_t)(device->left_stick_y + INT16_MIN) * -2 / UINT16_MAX + 1.0f
        };
    }
    else {
        return (CpdGamepadStickPosition) {
            .x = (float)(uint16_t)(device->right_stick_x + INT16_MIN) * 2 / UINT16_MAX - 1.0f,
            .y = (float)(uint16_t)(device->right_stick_y + INT16_MIN) * -2 / UINT16_MAX + 1.0f
        };
    }
}

CpdGamepadTriggersPosition get_gamepad_triggers_position(CpdWindow window, uint16_t gamepad_index) {
    CpdEventDevice* device = g_event_devices;

    for (uint16_t i = 0; i < gamepad_index; i++) {
        device = device->next;
    }

    return (CpdGamepadTriggersPosition) {
        .left = (float)device->left_trigger / 1023,
        .right= (float)device->right_trigger / 1023
    };
}
