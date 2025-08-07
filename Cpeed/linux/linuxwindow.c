#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "linuxinput.h"
#include "linuxmain.h"

static int g_windows_created;
static CpdWaylandWindowListNode* g_windows_list;

static struct wl_compositor* g_compositor;
static struct wl_registry* g_registry;
static struct xdg_wm_base* g_wm_base;

struct wl_display* g_display;

static void cleanup_input_queue(CpdInputEvent* events, uint32_t size);
static bool initialize_wayland();
static void registry_global(
    void* data, struct wl_registry* wl_registry, uint32_t name,
    const char* interface, uint32_t version
);
struct wl_registry_listener registry_listener = (struct wl_registry_listener) {
    .global = registry_global
};

static void wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial);
static struct xdg_wm_base_listener wm_base_listener = (struct xdg_wm_base_listener) {
    .ping = wm_base_ping
};

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
static struct wl_callback_listener frame_listener = (struct wl_callback_listener){
    .done = frame_done
};

static CpdWaylandWindowListNode* add_window_to_list(CpdWaylandWindow* window) {
    for (CpdWaylandWindowListNode* current_node = g_windows_list; current_node != 0; current_node = current_node->next) {
        if (current_node->window == window) {
            return current_node;
        }
    }

    CpdWaylandWindowListNode* node = (CpdWaylandWindowListNode*)malloc(sizeof(CpdWaylandWindowListNode));
    if (node == 0) {
        return 0;
    }

    node->previous = 0;
    node->next = 0;
    node->window = window;

    if (g_windows_list == 0) {
        g_windows_list = node;
        return node;
    }

    CpdWaylandWindowListNode* current_node = g_windows_list;
    while (current_node->next != 0) {
        current_node = current_node->next;
    }

    current_node->next = node;
    node->previous = current_node;

    return node;
}

static void remove_window_from_list(CpdWaylandWindow* window) {
    if (g_windows_list == 0) {
        return;
    }

    if (g_windows_list->next == 0 && g_windows_list->window == window) {
        free(g_windows_list);
        g_windows_list = 0;
        return;
    }

    CpdWaylandWindowListNode* current_node = g_windows_list;
    do {
        if (current_node->window == window) {
            if (current_node->previous != 0) {
                current_node->previous->next = current_node->next;
            }

            if (current_node->next != 0) {
                current_node->next->previous = current_node->previous;
            }

            free(current_node);
            break;
        }

        current_node = current_node->next;
    } while (current_node != 0);
}

CpdWaylandWindow* find_window_by_surface(struct wl_surface* surface) {
    for (CpdWaylandWindowListNode* current_node = g_windows_list; current_node != 0; current_node = current_node->next) {
        if (current_node->window->surface == surface) {
            return current_node->window;
        }
    }

    return 0;
}

CpdWindow PLATFORM_create_window(const CpdWindowInfo* info) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)malloc(sizeof(CpdWaylandWindow));
    if (wl_window == 0) {
        printf("%s", "Unable to allocate window\n");
        return 0;
    }

    CpdInputEvent* input_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    if (input_queue == 0 || input_swap_queue == 0) {
        free(input_queue);
        free(input_swap_queue);
        free(wl_window);
        printf("%s", "Unable to allocate window data\n");
        return 0;
    }

    CpdWaylandWindowListNode* node = add_window_to_list(wl_window);
    if (node == 0) {
        free(input_queue);
        free(input_swap_queue);
        free(wl_window);
        printf("%s", "Unable to allocate list node for window\n");
        return 0;
    }

    if (g_compositor == 0 && !initialize_wayland()) {
        remove_window_from_list(wl_window);

        free(input_queue);
        free(input_swap_queue);
        free(wl_window);
        return 0;
    }

    struct wl_surface* surface = wl_compositor_create_surface(g_compositor);

    struct wl_callback* callback = wl_surface_frame(surface);
    wl_callback_add_listener(callback, &frame_listener, (void*)wl_window);

    struct xdg_surface* shell_surface = xdg_wm_base_get_xdg_surface(g_wm_base, surface);
    xdg_surface_add_listener(shell_surface, &surface_listener, 0);

    struct xdg_toplevel* top_level = xdg_surface_get_toplevel(shell_surface);
    xdg_toplevel_add_listener(top_level, &top_level_listener, (void*)wl_window);

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

    wl_window->resized = false;
    wl_window->should_close = false;
    wl_window->should_render = true;
    wl_window->resize_swap_queue = false;
    wl_window->first_mouse_event = true;

    xdg_toplevel_set_title(top_level, info->title);
    xdg_toplevel_set_app_id(top_level, "Cpeed");

    wl_surface_commit(surface);
    wl_display_roundtrip(g_display);
    wl_surface_commit(surface);

    g_windows_created++;

    return (CpdWindow)wl_window;
}

void PLATFORM_window_destroy(CpdWindow window) {
    if (g_windows_created == 0) {
        return;
    }

    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    if (g_current_pointer_focus == wl_window) {
        g_current_pointer_focus = 0;
    }

    if (g_current_keyboard_focus == wl_window) {
        g_current_keyboard_focus = 0;
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
    
    if (--g_windows_created == 0) {
        xdg_wm_base_destroy(g_wm_base);
        g_wm_base = 0;

        destroy_pointer();
        destroy_keyboard();

        wl_seat_destroy(g_seat);
        g_seat = 0;

        wl_compositor_destroy(g_compositor);
        g_compositor = 0;

        wl_registry_destroy(g_registry);
        g_registry = 0;

        wl_display_disconnect(g_display);
        g_display = 0;
    }
}

void PLATFORM_window_close(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    wl_window->should_close = true;
}

bool PLATFORM_window_poll(CpdWindow window) {
    if (g_display == 0) {
        return true;
    }

    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    if (wl_window->should_close) {
        return true;
    }

    // This call is required as sometimes there are still some
    // events present in the default queue
    wl_display_dispatch_pending(g_display);

    if (wl_display_prepare_read(g_display) == -1) {
        printf("Unable to prepare for reading display events (%d)\n", errno);
        return true;
    }

    if (wl_display_read_events(g_display) == -1) {
        printf("Unable to read display events (%d)\n", errno);
        return true;
    }

    if (wl_display_dispatch_pending(g_display) == -1) {
        printf("%s", "Unable to dispatch events for Wayland display\n");
        return true;
    }

    if (wl_display_flush(g_display) == -1) {
        printf("%s", "Unable to flush display data\n");
        return true;
    }

    return wl_window->should_close;
}

CpdSize PLATFORM_get_window_size(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return (CpdSize) {
        .width = (unsigned short)wl_window->width,
        .height = (unsigned short)wl_window->height
    };
}

bool PLATFORM_window_resized(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    bool result = wl_window->resized;
    wl_window->resized = false;

    return result;
}

bool PLATFORM_window_present_allowed(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    bool result = wl_window->should_render;
    wl_window->should_render = false;

    return result;
}

bool PLATFORM_set_input_mode(CpdWindow window, CpdInputMode mode) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    if (wl_window->input_queue_size != 0) {
        return false;
    }

    wl_window->input_mode = mode;
    return true;
}

CpdInputMode PLATFORM_get_input_mode(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    return wl_window->input_mode;
}

bool PLATFORM_get_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

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

    PLATFORM_clear_event_queue(window);

    return true;
}

void PLATFORM_clear_event_queue(CpdWindow window) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    cleanup_input_queue(wl_window->input_queue, wl_window->input_queue_size);

    wl_window->input_queue_size = 0;
}

static void cleanup_input_queue(CpdInputEvent* events, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        CpdInputEvent* event = &events[i];

        if (event->type == CpdInputEventType_TextInput) {
            free(event->data.text_input.text);
            event->data.text_input.text = 0;
        }
    }
}

static void registry_global(
    void* data, struct wl_registry* wl_registry, uint32_t name,
    const char* interface, uint32_t version
) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        g_compositor = (struct wl_compositor*)wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        g_seat = (struct wl_seat*)wl_registry_bind(wl_registry, name, &wl_seat_interface, version);

        wl_seat_add_listener(g_seat, &seat_listener, 0);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        g_wm_base = (struct xdg_wm_base*)wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);

        xdg_wm_base_add_listener(g_wm_base, &wm_base_listener, 0);
    }
}

static bool initialize_wayland() {
    g_display = wl_display_connect(0);

    if (g_display == 0) {
        printf("%s", "Unable to connect to Wayland display\n");
        return false;
    }

    g_registry = wl_display_get_registry(g_display);

    wl_registry_add_listener(g_registry, &registry_listener, 0);
    wl_display_roundtrip(g_display);

    if (g_compositor == 0) {
        printf("%s", "Unable to find compositor for Wayland display\n");
        return false;
    }

    if (g_wm_base == 0) {
        printf("%s", "Unable to find window manager for Wayland display\n");
        return false;
    }

    return true;
}

static void wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static void surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
}

static void top_level_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states) {
    top_level_configure_bounds(data, xdg_toplevel, width, height);
}

static void top_level_close(void* data, struct xdg_toplevel* xdg_toplevel) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    wl_window->should_close = true;
}

static void top_level_configure_bounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    if (width != 0 && height != 0) {
        wl_window->width = min_i32(width, wl_window->width);
        wl_window->height = min_i32(height, wl_window->height);
        wl_window->resized = true;
    }
}

static void top_level_wm_capabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities) {
    if (capabilities->size == 0) {
        return;
    }

    printf("%s", "Window capabilities:");

    for (uint32_t i = 0; i < capabilities->size; i++) {
        printf(" %d", ((uint32_t*)capabilities->data)[i]);
    }

    printf("%s", "\n");
}

static void frame_done(void* data, struct wl_callback* wl_callback, uint32_t callback_data) {
    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)data;

    wl_callback_destroy(wl_callback);

    wl_window->callback = wl_surface_frame(wl_window->surface);

    wl_callback_add_listener(wl_window->callback, &frame_listener, data);

    wl_window->should_render = true;
}
