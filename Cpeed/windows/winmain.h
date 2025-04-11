#pragma once

#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <malloc.h>

#include "../vulkan.h"
#include "../platform.h"

#include <windowsx.h>

typedef struct WindowExtraData {
    bool should_close;
    bool resized;
} WindowExtraData;
