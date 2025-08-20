#pragma once

#include <stdint.h>

#include "vulkan.h"
#include "platform/window.h"

typedef enum CpdCompilePlatform {
    CpdCompilePlatform_Windows,
    CpdCompilePlatform_Linux
} CpdCompilePlatform;

typedef struct CpdPlatformExtensions {
    const char** extensions;
    uint32_t count;
} CpdPlatformExtensions;

extern CpdCompilePlatform compile_platform();

extern uint64_t get_clock_usec();

// == Vulkan

extern PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib();
extern void PLATFORM_free_vulkan_lib();

extern const CpdPlatformExtensions* PLATFORM_alloc_vulkan_instance_extensions();
extern void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions);

extern VkResult PLATFORM_create_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* result);
