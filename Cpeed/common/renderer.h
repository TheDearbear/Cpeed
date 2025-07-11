#pragma once

#include "../vulkan.h"
#include "device.h"
#include "swapchain.h"
#include "surface.h"

typedef struct CpdRenderer {
    VkInstance instance;

    uint32_t api_version;
    uint32_t target_version;

    uint64_t creation_time;
    uint64_t last_frame_end;

    CpdInstanceVulkanExtensions instance_extensions;

    CpdSurface surface;

    CpdSwapchain swapchain;
    VkFence swapchain_image_fence;

    CpdDevice render_device;
    CpdDevice ui_device;
} CpdRenderer;

typedef struct CpdRendererInitParams {
    VkInstance instance;
    CpdInstanceVulkanExtensions* instance_extensions;
    uint32_t api_version;
    uint32_t max_api_version;
} CpdRendererInitParams;

CpdRenderer* RENDERER_create(CpdRendererInitParams* params);
void RENDERER_destroy(CpdRenderer* renderer);

VkResult RENDERER_select_render_device(CpdRenderer* renderer);
VkResult RENDERER_select_ui_device(CpdRenderer* renderer);

VkResult RENDERER_reset_pools(CpdRenderer* renderer);
VkResult RENDERER_wait_idle(CpdRenderer* renderer);

VkResult RENDERER_acquire_next_image(CpdRenderer* renderer, bool wait_for_fence);
