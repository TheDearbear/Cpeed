#pragma once

#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <malloc.h>

#include "../vulkan.h"
#include "../platform.h"

#include <windowsx.h>

#define WND_OFFSET_SHOULD_CLOSE 0
#define WND_OFFSET_RESIZED 4
