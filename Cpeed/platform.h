#pragma once

#include <vulkan/vulkan.h>

extern PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib();
extern void PLATFORM_free_vulkan_lib();
