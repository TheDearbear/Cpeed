#include "../platform/input.h"
#include "winmain.h"

bool set_window_input_mode(CpdWindow window, CpdInputMode mode) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    if (data->input_queue_size != 0) {
        return false;
    }

    data->input_mode = mode;
    return true;
}

CpdInputMode get_window_input_mode(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return data->input_mode;
}

bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

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

    clear_window_event_queue(window);

    return true;
}

void clear_window_event_queue(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    cleanup_input_queue(data->input_queue, data->input_queue_size);

    data->input_queue_size = 0;
}
