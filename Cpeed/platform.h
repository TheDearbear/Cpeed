#pragma once

#include <stdbool.h>

#include "input.h"
#include "vulkan.h"
#include "window.h"

typedef enum CpdCompilePlatform {
    CpdCompilePlatform_Windows,
    CpdCompilePlatform_Linux
} CpdCompilePlatform;

typedef struct CpdPlatformExtensions {
    const char** extensions;
    uint32_t count;
} CpdPlatformExtensions;

// == Generic

extern CpdCompilePlatform PLATFORM_compile_platform();

extern bool PLATFORM_initialize();

extern void PLATFORM_shutdown();

extern uint64_t PLATFORM_get_clock_usec();

// == Vulkan

extern PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib();
extern void PLATFORM_free_vulkan_lib();

extern const CpdPlatformExtensions* PLATFORM_alloc_vulkan_instance_extensions();
extern void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions);

extern VkResult PLATFORM_create_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* result);

// == Windowing

extern CpdWindow PLATFORM_create_window(const CpdWindowInfo* info);
extern void PLATFORM_window_destroy(CpdWindow window);

extern void PLATFORM_window_close(CpdWindow window);

extern bool PLATFORM_window_poll(CpdWindow window);

extern CpdSize PLATFORM_get_window_size(CpdWindow window);
extern bool PLATFORM_window_resized(CpdWindow window);

extern bool PLATFORM_window_present_allowed(CpdWindow window);

// == Input

extern bool PLATFORM_set_input_mode(CpdWindow window, CpdInputMode mode);
extern CpdInputMode PLATFORM_get_input_mode(CpdWindow window);

extern bool PLATFORM_get_events(CpdWindow window, const CpdInputEvent** events, uint32_t* size);
extern void PLATFORM_clear_event_queue(CpdWindow window);
