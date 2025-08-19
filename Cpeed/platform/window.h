#pragma once

#include <stdbool.h>

#include "../common/general.h"
#include "../input/keyboard.h"

typedef void* CpdWindow;

// Reserved for future use
typedef unsigned int CpdWindowFlags;

typedef struct CpdWindowInfo {
    const char* title;
    CpdSize size;
    CpdWindowFlags flags;
    CpdInputMode input_mode;
} CpdWindowInfo;

extern CpdWindow PLATFORM_create_window(const CpdWindowInfo* info);
extern void PLATFORM_window_destroy(CpdWindow window);

extern void PLATFORM_window_close(CpdWindow window);

extern bool PLATFORM_window_poll(CpdWindow window);

extern CpdSize PLATFORM_get_window_size(CpdWindow window);
extern bool PLATFORM_window_resized(CpdWindow window);

extern bool PLATFORM_window_present_allowed(CpdWindow window);
