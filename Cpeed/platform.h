#pragma once

#include "common/window.h"

#include <stdbool.h>
#include <vulkan/vulkan.h>

typedef enum CpdCompilePlatform {
    CpdCompilePlatform_Windows,
    CpdCompilePlatform_Linux
} CpdCompilePlatform;

typedef struct CpdPlatformExtensions {
    char** extensions;
    unsigned int count;
} CpdPlatformExtensions;

// == Generic

extern CpdCompilePlatform PLATFORM_compile_platform();

// == Vulkan

extern PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib();
extern void PLATFORM_free_vulkan_lib();

extern const CpdPlatformExtensions* PLATFORM_alloc_vulkan_instance_extensions();
extern const CpdPlatformExtensions* PLATFORM_alloc_vulkan_render_device_extensions();
extern const CpdPlatformExtensions* PLATFORM_alloc_vulkan_ui_device_extensions();

extern VkResult PLATFORM_create_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* result);

extern void PLATFORM_free_vulkan_extensions(CpdPlatformExtensions* extensions);

// == Windowing

extern CpdWindow PLATFORM_create_window(const CpdWindowInfo* info);
extern void PLATFORM_window_destroy(CpdWindow window);

extern void PLATFORM_window_show(CpdWindow window);
extern void PLATFORM_window_hide(CpdWindow window);
extern bool PLATFORM_window_poll(CpdWindow window);
