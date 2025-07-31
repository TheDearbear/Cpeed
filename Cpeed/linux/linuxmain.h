#pragma once

#define VK_USE_PLATFORM_WAYLAND_KHR

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "xdg-shell/client.h"

#include "../vulkan.h"
#include "../platform.h"

extern struct wl_display* g_display;

typedef struct CpdWaylandWindow {
    struct wl_surface* surface;
    struct wl_callback* callback;
    struct xdg_surface* shell_surface;
    struct xdg_toplevel* top_level;
    int32_t width;
    int32_t height;
    bool resized;
    bool should_close;
    bool should_render;
} CpdWaylandWindow;
