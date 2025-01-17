#pragma once

#include "common/window.h"

#include <stdbool.h>
#include <vulkan/vulkan.h>

extern PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib();
extern void PLATFORM_free_vulkan_lib();

extern CpdWindow PLATFORM_create_window(const CpdWindowInfo* info);
extern void PLATFORM_window_show(CpdWindow window);
extern void PLATFORM_window_hide(CpdWindow window);
extern void PLATFORM_window_destroy(CpdWindow window);
extern bool PLATFORM_window_poll(CpdWindow window);
