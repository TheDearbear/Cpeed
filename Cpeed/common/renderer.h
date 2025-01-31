#pragma once

#include "../vulkan.h"
#include "../platform.h"
#include "renderer.queue_init.h"

typedef struct CpdRenderSurface {
    VkSurfaceKHR handle;
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR format;
} CpdRenderSurface;

typedef struct CpdTransferQueue {
    uint64_t bytes_queued;
    VkQueue handle;
} CpdTransferQueue;

typedef struct CpdQueueFamily {
    VkQueue queue;
    uint32_t index;
} CpdQueueFamily;

typedef struct CpdTransferQueueFamily {
    CpdTransferQueue* queues;
    uint32_t queue_count;
    uint32_t index;
} CpdTransferQueueFamily;

typedef struct CpdDevice {
    VkPhysicalDevice physical_handle;
    VkDevice handle;

    CpdQueueFamily graphics_family;
    CpdQueueFamily compute_family;
    CpdTransferQueueFamily transfer_family;

    // == Logical Device

    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkDestroyDevice vkDestroyDevice;

    // == Swapchain

    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;

    // == Queue

    PFN_vkQueueWaitIdle vkQueueWaitIdle;
} CpdDevice;

typedef struct CpdRenderer {
    VkInstance instance;
    CpdRenderSurface surface;

    CpdDevice render_device;
    CpdDevice ui_device;
} CpdRenderer;

CpdRenderer* RENDERER_create(VkInstance instance);
void RENDERER_destroy(CpdRenderer* renderer);
VkResult RENDERER_set_surface(CpdRenderer* renderer, VkSurfaceKHR surface, CpdWindowSize* size);
VkResult RENDERER_select_render_device(CpdRenderer* renderer);
VkResult RENDERER_select_ui_device(CpdRenderer* renderer);
VkResult RENDERER_update_surface_size(CpdRenderer* renderer, CpdWindowSize* size);
