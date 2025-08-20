#include <malloc.h>

#include "../platform/input.h"
#include "linuxmain.h"
#include "linuxwayland.h"

bool set_window_input_mode(CpdWindow window, CpdInputMode mode) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    if (wl_window->input_queue_size != 0) {
        return false;
    }

    wl_window->input_mode = mode;
    return true;
}

CpdInputMode get_window_input_mode(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return wl_window->input_mode;
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

void clear_window_event_queue(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    cleanup_input_queue(wl_window->input_queue, wl_window->input_queue_size);

    wl_window->input_queue_size = 0;
}
