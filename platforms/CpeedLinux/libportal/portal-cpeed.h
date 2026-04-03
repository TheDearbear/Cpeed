#pragma once

#include <Cpeed/platform/window.h>

#include <libportal/types.h>

#define CPEED_WINDOW_TYPE g_cpeed_window_get_type()
G_DECLARE_FINAL_TYPE(GCpeedWindow, g_cpeed_window, CPEED, WINDOW, GObject)

XdpParent* xdp_parent_new_cpeed(CpdWindow window);
