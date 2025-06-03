#pragma once

#include "../vulkan.h"
#include "device.h"
#include "swapchain.h"
#include "surface.h"

typedef struct CpdRenderer {
    VkInstance instance;
    CpdSurface surface;

    CpdSwapchain swapchain;
    VkFence swapchain_image_fence;

    CpdDevice render_device;
    CpdDevice ui_device;
} CpdRenderer;

CpdRenderer* RENDERER_create(VkInstance instance);
void RENDERER_destroy(CpdRenderer* renderer);

VkResult RENDERER_select_render_device(CpdRenderer* renderer);
VkResult RENDERER_select_ui_device(CpdRenderer* renderer);

VkResult RENDERER_reset_pools(CpdRenderer* renderer);
VkResult RENDERER_wait_idle(CpdRenderer* renderer);

uint32_t RENDERER_acquire_next_image(CpdRenderer* renderer, bool* should_wait_for_fence);
