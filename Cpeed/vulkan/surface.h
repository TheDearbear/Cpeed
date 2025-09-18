#pragma once

#include "device.h"
#include "../common/general.h"

typedef struct CpdSurface {
    VkSurfaceKHR handle;
    VkSurfaceFormatKHR format;
} CpdSurface;

VkResult SURFACE_initialize(CpdSurface* cpeed_surface, CpdDevice* cpeed_device, VkSurfaceKHR surface);
