#pragma once

#define VK_USE_PLATFORM_WAYLAND_KHR

#include <xkbcommon/xkbcommon.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "xdg-shell/client.h"

#include "../vulkan.h"
#include "../platform.h"

#define INPUT_QUEUE_BASE_SIZE 16
#define INPUT_QUEUE_SIZE_STEP 16

extern struct wl_display* g_display;
extern struct xkb_context* g_xkb_context;

typedef struct CpdWaylandWindow {
    struct wl_surface* surface;
    struct wl_callback* callback;
    struct xdg_surface* shell_surface;
    struct xdg_toplevel* top_level;

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

    uint64_t resized : 1;
    uint64_t should_close : 1;
    uint64_t should_render : 1;
    uint64_t resize_swap_queue : 1;
    uint64_t first_mouse_event : 1;
    uint64_t maximized : 1;
} CpdWaylandWindow;

typedef struct CpdWaylandWindowListNode {
    struct CpdWaylandWindowListNode* previous;
    struct CpdWaylandWindowListNode* next;
    CpdWaylandWindow* window;
} CpdWaylandWindowListNode;

typedef struct CpdWaylandKeyboard {
    struct wl_keyboard* keyboard;
    struct xkb_keymap* keymap;
    struct xkb_state* state;

    CpdInputModifierKeyFlags modifiers;

    uint32_t mods_depressed;
    uint32_t mods_latched;
    uint32_t mods_locked;
    uint32_t group;
} CpdWaylandKeyboard;

extern CpdWaylandWindow* find_window_by_surface(struct wl_surface* surface);
