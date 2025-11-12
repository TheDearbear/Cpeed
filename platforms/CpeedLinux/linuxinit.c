#include <string.h>
#include <xkbcommon/xkbcommon.h>

#include <Cpeed/platform/init.h>

#include "linuxevent.h"
#include "linuxwayland.h"

static struct wl_registry* g_registry;

static void wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static struct xdg_wm_base_listener wm_base_listener = (struct xdg_wm_base_listener) {
    .ping = wm_base_ping
};

static void registry_global(
    void* data, struct wl_registry* wl_registry, uint32_t name,
    const char* interface, uint32_t version
) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        g_compositor = (struct wl_compositor*)wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0) {
        g_seat = (struct wl_seat*)wl_registry_bind(wl_registry, name, &wl_seat_interface, version);

        wl_seat_add_listener(g_seat, &g_seat_listener, 0);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        g_wm_base = (struct xdg_wm_base*)wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);

        xdg_wm_base_add_listener(g_wm_base, &wm_base_listener, 0);
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        g_decoration = (struct zxdg_decoration_manager_v1*)wl_registry_bind(wl_registry, name, &zxdg_decoration_manager_v1_interface, 1);
    }
}

struct wl_registry_listener registry_listener = (struct wl_registry_listener) {
    .global = registry_global
};

bool initialize_platform() {
    g_xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (g_xkb_context == 0) {
        return false;
    }

    if (!init_events()) {
        return false;
    }

    g_display = wl_display_connect(0);

    if (g_display == 0) {
        return false;
    }

    g_registry = wl_display_get_registry(g_display);

    wl_registry_add_listener(g_registry, &registry_listener, 0);
    wl_display_roundtrip(g_display);

    return g_compositor != 0 && g_wm_base != 0;
}

void shutdown_platform() {
    if (g_xkb_context != 0) {
        xkb_context_unref(g_xkb_context);
        g_xkb_context = 0;
    }

    shutdown_events();

    if (g_decoration != 0) {
        zxdg_decoration_manager_v1_destroy(g_decoration);
        g_decoration = 0;
    }

    if (g_wm_base != 0) {
        xdg_wm_base_destroy(g_wm_base);
        g_wm_base = 0;
    }

    destroy_pointer();
    destroy_keyboard();

    if (g_seat != 0) {
        wl_seat_destroy(g_seat);
        g_seat = 0;
    }

    if (g_compositor != 0) {
        wl_compositor_destroy(g_compositor);
        g_compositor = 0;
    }

    if (g_registry != 0) {
        wl_registry_destroy(g_registry);
        g_registry = 0;
    }

    if (g_display != 0) {
        wl_display_disconnect(g_display);
        g_display = 0;
    }
}
