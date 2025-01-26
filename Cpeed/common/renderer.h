#pragma once

#include <malloc.h>

#include "../vulkan.h"
#include "../platform.h"
#include "renderer.queue_init.h"

typedef struct CpdTransferQueue {
    uint64_t bytes_queued;
    VkQueue handle;
} CpdTransferQueue;

typedef struct CpdDevice {
    VkPhysicalDevice physical_handle;
    VkDevice handle;

    uint32_t graphics_index;
    VkQueue graphics_queue;

    uint32_t compute_index;
    VkQueue compute_queue;

    uint32_t transfer_index;
    uint32_t transfer_queue_count;
    CpdTransferQueue* transfer_queues;

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
