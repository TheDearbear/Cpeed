#include <malloc.h>

#include "linuxwindowlist.h"

static CpdWaylandWindowListNode* g_windows_list;

CpdWaylandWindowListNode* add_window_to_list(CpdWaylandWindow* window) {
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

void remove_window_from_list(CpdWaylandWindow* window) {
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
