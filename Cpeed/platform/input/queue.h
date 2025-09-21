#pragma once

#include "../../input.h"
#include "../window.h"

extern bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size);
extern void clear_window_event_queue(CpdWindow window);
