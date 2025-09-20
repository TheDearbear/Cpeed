#pragma once

#include <stdint.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "xdg-decoration/client.h"
#include "xdg-shell/client.h"

#include "../input.h"

#define INPUT_QUEUE_BASE_SIZE 16
#define INPUT_QUEUE_SIZE_STEP 16

typedef struct CpdWaylandWindow {
    struct wl_surface* surface;
    struct wl_callback* callback;
    struct xdg_surface* shell_surface;
    struct xdg_toplevel* top_level;
    struct zxdg_toplevel_decoration_v1* decoration;

    CpdInputMode input_mode;
    uint32_t input_queue_size;
    uint32_t input_swap_queue_size;
    uint32_t input_queue_max_size;
    CpdInputEvent* input_queue;
    CpdInputEvent* input_swap_queue;

    int32_t width;
    int32_t height;

    int32_t mouse_x;
    int32_t mouse_y;

    uint64_t last_repeating_key_events_insert_time;

    uint64_t resized : 1;
    uint64_t should_close : 1;
    uint64_t should_render : 1;
    uint64_t resize_swap_queue : 1;
    uint64_t first_mouse_event : 1;
    uint64_t maximized : 1;
} CpdWaylandWindow;

void cleanup_input_queue(CpdInputEvent* events, uint32_t size);
bool resize_input_queue_if_need(CpdWaylandWindow* window, uint32_t new_events);
