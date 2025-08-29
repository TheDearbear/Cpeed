#pragma once

#include <vulkan/vulkan.h>

#include "../window.h"

typedef struct CpdVulkanExtensions {
    const char** extensions;
    uint32_t count;
} CpdVulkanExtensions;

extern PFN_vkGetInstanceProcAddr load_vulkan_lib();
extern void free_vulkan_lib();

extern const CpdVulkanExtensions* alloc_vulkan_instance_extensions();
extern void free_vulkan_extensions(const CpdVulkanExtensions* extensions);

extern VkResult create_vulkan_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* result);
