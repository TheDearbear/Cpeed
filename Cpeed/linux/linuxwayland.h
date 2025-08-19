#pragma once

#include <xkbcommon/xkbcommon.h>

#include "linuxmain.h"

extern CpdWaylandWindow* g_current_pointer_focus;
extern CpdWaylandWindow* g_current_keyboard_focus;

extern struct wl_compositor* g_compositor;
extern struct wl_display* g_display;
extern struct wl_seat* g_seat;
extern struct xdg_wm_base* g_wm_base;

extern struct wl_seat_listener g_seat_listener;

extern struct xkb_context* g_xkb_context;

typedef struct CpdPressedButton {
    struct CpdPressedButton* next;
    uint64_t time;
    CpdKeyCode key_code;
} CpdPressedButton;

typedef struct CpdWaylandKeyboard {
    struct wl_keyboard* keyboard;
    struct xkb_keymap* keymap;
    struct xkb_state* state;

    CpdPressedButton* pressed_buttons;

    CpdInputModifierKeyFlags modifiers;

    uint32_t mods_depressed;
    uint32_t mods_latched;
    uint32_t mods_locked;
    uint32_t group;

    int32_t repeat_delay;
    int32_t repeat_rate;
    bool server_side_repeating;
} CpdWaylandKeyboard;

extern void destroy_pointer();
extern void destroy_keyboard();
