#pragma once

#include "general.h"
#include "image.h"
#include "surface.h"

typedef struct CpdSwapchainImage {
    CpdImage image;
    VkSemaphore semaphore;
} CpdSwapchainImage;

typedef struct CpdSwapchain {
    VkSwapchainKHR handle;
    CpdSize size;
    CpdSwapchainImage* images;
    uint32_t image_count;
    uint32_t current_image;
} CpdSwapchain;


void SWAPCHAIN_destroy(CpdSwapchain* swapchain, CpdDevice* cpeed_device);
VkResult SWAPCHAIN_create(CpdSwapchain* swapchain, CpdDevice* cpeed_device, CpdSurface* surface, CpdSize* size);

VkResult SWAPCHAIN_resize(CpdSwapchain* swapchain, CpdDevice* cpeed_device, CpdSurface* surface, CpdSize* size);
void SWAPCHAIN_set_layout(CpdSwapchain* swapchain, CpdDevice* cpeed_device, VkCommandBuffer buffer,
    VkPipelineStageFlags2KHR stage, VkAccessFlags2KHR access, VkImageLayout layout);
