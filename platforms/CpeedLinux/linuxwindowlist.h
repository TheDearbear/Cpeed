#pragma once

#include "linuxmain.h"

typedef struct CpdWaylandWindowListNode {
    struct CpdWaylandWindowListNode* previous;
    struct CpdWaylandWindowListNode* next;
    CpdWaylandWindow* window;
} CpdWaylandWindowListNode;

extern CpdWaylandWindowListNode* g_windows_list;

CpdWaylandWindowListNode* add_window_to_list(CpdWaylandWindow* window);

void remove_window_from_list(CpdWaylandWindow* window);

CpdWaylandWindow* find_window_by_surface(struct wl_surface* surface);
