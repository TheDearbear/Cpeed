#pragma once

#include "linuxmain.h"

extern CpdWaylandWindow* g_current_pointer_focus;
extern CpdWaylandWindow* g_current_keyboard_focus;

extern struct wl_seat* g_seat;
extern struct wl_seat_listener seat_listener;

extern void destroy_pointer();
extern void destroy_keyboard();
