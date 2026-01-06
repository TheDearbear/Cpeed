#include <malloc.h>

#include <Cpeed/input/gamepad.h>
#include <Cpeed/platform/window.h>
#include <Cpeed/input.h>

bool set_window_input_mode(CpdWindow window, CpdInputMode mode) {
    //WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    //data->input_mode = mode;
    return true;
}

CpdInputMode get_window_input_mode(CpdWindow window) {
    //WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return CpdInputMode_KeyCode;//return data->input_mode;
}

void clear_window_event_queue(CpdWindow window) {
    //WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    //cleanup_input_queue(data->input_queue, data->input_queue_size);

    //data->input_queue_size = 0;
}

bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size) {
    /*WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    if (data->input_queue_size == 0) {
        return false;
    }

    if (data->resize_swap_queue) {
        CpdInputEvent* swap_queue = (CpdInputEvent*)malloc(data->input_queue_max_size * sizeof(CpdInputEvent));

        if (swap_queue == 0) {
            return false;
        }

        free(data->input_swap_queue);
        data->input_swap_queue = swap_queue;
        data->resize_swap_queue = false;
    }

    *events = data->input_queue;
    *size = data->input_queue_size;

    uint32_t temp_size = data->input_queue_size;
    data->input_queue_size = data->input_swap_queue_size;
    data->input_swap_queue_size = temp_size;

    CpdInputEvent* temp_queue = data->input_queue;
    data->input_queue = data->input_swap_queue;
    data->input_swap_queue = temp_queue;

    clear_window_event_queue(window);*/
    *size = 0;

    return true;
}

uint16_t get_gamepad_count(CpdWindow window) {
    /*CpdGamepad* current = g_gamepads;
    uint16_t count = 0;

    while (current != 0) {
        current = current->next;
        count++;
    }

    return count;*/
    return 0;
}

CpdGamepadStickPosition get_gamepad_stick_position(CpdWindow window, uint16_t gamepad_index, CpdGamepadStick stick) {
    /*CpdGamepad* current = g_gamepads;

    for (uint16_t i = 0; i < gamepad_index; i++) {
        current = current->next;
    }

    if (stick == CpdGamepadStick_Left) {
        return (CpdGamepadStickPosition) {
            .x = current->last_used_state.leftThumbstickX,
                .y = current->last_used_state.leftThumbstickY,
        };
    }
    else {
        return (CpdGamepadStickPosition) {
            .x = current->last_used_state.rightThumbstickX,
                .y = current->last_used_state.rightThumbstickY,
        };
    }*/

    return (CpdGamepadStickPosition) {
        .x = 0.0f,
        .y = 0.0f
    };
}

CpdGamepadTriggersPosition get_gamepad_triggers_position(CpdWindow window, uint16_t gamepad_index) {
    //CpdGamepad* current = g_gamepads;

    //for (uint16_t i = 0; i < gamepad_index; i++) {
    //    current = current->next;
    //}

    return (CpdGamepadTriggersPosition) {
        .left = 0.0f,//current->last_used_state.leftTrigger,
        .right = 0.0f//current->last_used_state.rightTrigger
    };
}
