#pragma once

#include <stdbool.h>

#include "../input.h"
#include "window.h"

extern bool set_window_input_mode(CpdWindow window, CpdInputMode mode);
extern CpdInputMode get_window_input_mode(CpdWindow window);

extern bool get_window_input_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size);
extern void clear_window_event_queue(CpdWindow window);
