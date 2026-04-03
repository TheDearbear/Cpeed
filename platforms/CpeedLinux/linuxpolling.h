#pragma once

#include <glib.h>
#include <stdint.h>

extern GArray* g_poll_fds;

uint32_t poll_glib_events(void* context);
