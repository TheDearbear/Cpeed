#include <errno.h>
#include <malloc.h>

#include <Cpeed/platform/input/gamepad.h>
#include <Cpeed/platform/frame.h>
#include <Cpeed/platform/logging.h>
#include <Cpeed/platform/window.h>
#include <Cpeed/input.h>

#include "xdg-decoration/client.h"

#include "linuxevent.h"
#include "linuxmain.h"
#include "linuxwayland.h"
#include "linuxwindowlist.h"

static void surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial);
static struct xdg_surface_listener surface_listener = (struct xdg_surface_listener) {
    .configure = surface_configure
};

static void top_level_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states);
static void top_level_close(void* data, struct xdg_toplevel* xdg_toplevel);
static void top_level_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height);
static void top_level_wm_capabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities);
static struct xdg_toplevel_listener top_level_listener = (struct xdg_toplevel_listener) {
    .configure = top_level_configure,
    .close = top_level_close,
    .configure_bounds = top_level_configure_bounds,
    .wm_capabilities = top_level_wm_capabilities
};

static void frame_done(void* data, struct wl_callback* wl_callback, uint32_t callback_data);
static struct wl_callback_listener frame_listener = (struct wl_callback_listener) {
    .done = frame_done
};

static void decoration_configure(void* data, struct zxdg_toplevel_decoration_v1* zxdg_toplevel_decoration_v1, uint32_t mode);
static struct zxdg_toplevel_decoration_v1_listener decoration_listener = (struct zxdg_toplevel_decoration_v1_listener) {
    .configure = decoration_configure
};

CpdWindow create_window(const CpdWindowInfo* info) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)malloc(sizeof(CpdWaylandWindow));
    if (wl_window == 0) {
        log_error("%s", "Unable to allocate window\n");
        return 0;
    }

    CpdInputEvent* input_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    if (input_queue == 0 || input_swap_queue == 0) {
        free(input_queue);
        free(input_swap_queue);
        free(wl_window);
        log_error("%s", "Unable to allocate window data\n");
        return 0;
    }

    CpdWaylandWindowListNode* node = add_window_to_list(wl_window);
    if (node == 0) {
        free(input_queue);
        free(input_swap_queue);
        free(wl_window);
        log_error("%s", "Unable to allocate list node for window\n");
        return 0;
    }

    struct wl_surface* surface = wl_compositor_create_surface(g_compositor);

    struct wl_callback* callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener, (void*)wl_window);

    struct xdg_surface* shell_surface = xdg_wm_base_get_xdg_surface(g_wm_base, surface);
    xdg_surface_add_listener(shell_surface, &surface_listener, 0);

    struct xdg_toplevel* top_level = xdg_surface_get_toplevel(shell_surface);
    xdg_toplevel_add_listener(top_level, &top_level_listener, (void*)wl_window);

    if (g_decoration != 0) {
        wl_window->decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(g_decoration, top_level);
        zxdg_toplevel_decoration_v1_add_listener(wl_window->decoration, &decoration_listener, (void*)wl_window);
        zxdg_toplevel_decoration_v1_set_mode(wl_window->decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
    else {
        wl_window->decoration = 0;
    }

    wl_window->surface = surface;
    wl_window->callback = callback;
    wl_window->shell_surface = shell_surface;
    wl_window->top_level = top_level;

    wl_window->input_mode = info->input_mode;
    wl_window->input_queue_size = 0;
    wl_window->input_swap_queue_size = 0;
    wl_window->input_queue_max_size = INPUT_QUEUE_BASE_SIZE;
    wl_window->input_queue = input_queue;
    wl_window->input_swap_queue = input_swap_queue;

    wl_window->width = info->size.width;
    wl_window->height = info->size.height;

    wl_window->mouse_x = 0;
    wl_window->mouse_y = 0;

    wl_window->last_repeating_key_events_insert_time = 0;

    wl_window->layers = 0;

#ifdef CPD_IMGUI_AVAILABLE
    wl_window->imgui_context = ImGui_CreateContext(0);
    ImGui_SetCurrentContext(wl_window->imgui_context);
#endif

    wl_window->resized = false;
    wl_window->should_close = false;
    wl_window->should_render = true;
    wl_window->resize_swap_queue = false;
    wl_window->first_mouse_event = true;
    wl_window->maximized = false;

    xdg_toplevel_set_title(top_level, info->title);
    xdg_toplevel_set_app_id(top_level, "Cpeed");

    wl_surface_commit(surface);
    wl_display_roundtrip(g_display);
    wl_surface_commit(surface);

    uint16_t count = get_gamepad_count(wl_window);
    if (!resize_input_queue_if_need(wl_window, count)) {
        destroy_window(wl_window);
        return 0;
    }

    for (uint16_t i = 0; i < count; i++) {
        add_gamepad_connect_to_queue(wl_window, CpdGamepadConnectStatus_Connected, i);
    }

    return (CpdWindow)wl_window;
}

static bool remove_frame_layers_loop(void* context, CpdFrameLayer* layer) {
    CpdWindow window = (CpdWindow)context;

    remove_frame_layer(window, layer->handle);

    return true;
}

void destroy_window(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    loop_frame_layers(window, remove_frame_layers_loop, (void*)window);

#ifdef CPD_IMGUI_AVAILABLE
    ImGui_DestroyContext(wl_window->imgui_context);
#endif

    if (g_current_pointer_focus == wl_window) {
        g_current_pointer_focus = 0;
    }

    if (g_current_keyboard_focus == wl_window) {
        g_current_keyboard_focus = 0;
    }

    if (wl_window->decoration != 0) {
        zxdg_toplevel_decoration_v1_destroy(wl_window->decoration);
    }

    xdg_toplevel_destroy(wl_window->top_level);
    xdg_surface_destroy(wl_window->shell_surface);
    wl_callback_destroy(wl_window->callback);
    wl_surface_destroy(wl_window->surface);

    cleanup_input_queue(wl_window->input_queue, wl_window->input_queue_size);
    cleanup_input_queue(wl_window->input_swap_queue, wl_window->input_swap_queue_size);

    remove_window_from_list(wl_window);

    free(wl_window->input_queue);
    free(wl_window->input_swap_queue);
    free(wl_window);
}

void close_window(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    wl_window->should_close = true;
}

bool poll_window(CpdWindow window) {
    if (g_display == 0) {
        return true;
    }

    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    if (wl_window->should_close) {
        return true;
    }

    poll_events(wl_window);

    // This call is required as sometimes there are still some
    // events present in the default queue
    wl_display_dispatch_pending(g_display);

    if (wl_display_prepare_read(g_display) == -1) {
        log_error("Unable to prepare for reading display events (%d)\n", errno);
        return true;
    }

    if (wl_display_read_events(g_display) == -1) {
        log_error("Unable to read display events (%d)\n", errno);
        return true;
    }

    if (wl_display_dispatch_pending(g_display) == -1) {
        log_error("%s", "Unable to dispatch events for Wayland display\n");
        return true;
    }

    if (wl_display_flush(g_display) == -1) {
        log_error("%s", "Unable to flush display data\n");
        return true;
    }

    return wl_window->should_close;
}

CpdSize window_size(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return (CpdSize) {
        .width = (unsigned short)wl_window->width,
        .height = (unsigned short)wl_window->height
    };
}

bool window_resized(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    bool result = wl_window->resized;
    wl_window->resized = false;

    return result;
}

bool window_present_allowed(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    bool result = wl_window->should_render;
    wl_window->should_render = false;

    return result;
}

ImGuiContext* get_imgui_context(CpdWindow window) {
#ifdef CPD_IMGUI_AVAILABLE
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return wl_window->imgui_context;
#else
    return 0;
#endif
}

bool multiple_windows_supported() {
    return true;
}

bool windowed_mode_supported() {
    return true;
}

static void surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
}

static void top_level_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;
    bool maximized = false;

    const int32_t* state;
    wl_array_for_each(state, states) {
        if (*state == XDG_TOPLEVEL_STATE_MAXIMIZED) {
            maximized = true;
        }
#ifdef CPD_IMGUI_AVAILABLE
        else if (*state == XDG_TOPLEVEL_STATE_ACTIVATED) {
            ImGui_SetCurrentContext(wl_window->imgui_context);
        }
#endif
    }

    wl_window->maximized = maximized;

    top_level_configure_bounds(data, xdg_toplevel, width, height);
}

static void top_level_close(void* data, struct xdg_toplevel* xdg_toplevel) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    wl_window->should_close = true;
}

static void top_level_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    if (!wl_window->maximized) {
        width = min_i32(width, wl_window->width);
        height = min_i32(height, wl_window->height);
    }

    if (width != 0 && width != wl_window->width) {
        wl_window->width = width;
        wl_window->resized = true;
    }

    if (height != 0 && height != wl_window->height) {
        wl_window->height = height;
        wl_window->resized = true;
    }
}

static void top_level_wm_capabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities) {
    if (capabilities->size == 0) {
        return;
    }

    log_debug("%s", "Window capabilities:");

    const uint32_t* capability;
    wl_array_for_each(capability, capabilities) {
        log_debug(" %d", *capability);
    }

    log_debug("%s", "\n");
}

static void frame_done(void* data, struct wl_callback* wl_callback, uint32_t callback_data) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    wl_callback_destroy(wl_callback);

    wl_window->callback = wl_surface_frame(wl_window->surface);

    wl_callback_add_listener(wl_window->callback, &frame_listener, data);

    wl_window->should_render = true;
}

static void decoration_configure(void* data, struct zxdg_toplevel_decoration_v1* zxdg_toplevel_decoration_v1, uint32_t mode) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    if (mode != ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
        // TODO: Enable client-side decoration
    }
}
