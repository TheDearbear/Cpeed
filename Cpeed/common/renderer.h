#pragma once

#include "../main.h"

typedef struct CpdDevice {
    VkPhysicalDevice physical_handle;
    VkDevice handle;

    uint32_t graphics_index;
    VkQueue graphics_queue;

    uint32_t compute_index;
    VkQueue compute_queue;

    uint32_t transfer_index;
    VkQueue transfer_queue;

    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkDestroyDevice vkDestroyDevice;
} CpdDevice;

typedef struct CpdRenderer {
    VkInstance instance;
    CpdDevice render_device;
    CpdDevice ui_device;
} CpdRenderer;

CpdRenderer* RENDERER_create(VkInstance instance);
void RENDERER_destroy(CpdRenderer* renderer);
VkResult RENDERER_select_render_device(CpdRenderer* renderer);
VkResult RENDERER_select_ui_device(CpdRenderer* renderer);
