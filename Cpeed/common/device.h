#pragma once

#include <stdbool.h>

#include "../vulkan.h"
#include "../platform.h"

typedef struct CpdTransferQueue {
    uint64_t bytes_queued;
    VkQueue handle;
} CpdTransferQueue;

typedef struct CpdQueueFamily {
    VkQueue queue;
    uint32_t index;
    VkCommandPool pool;
} CpdQueueFamily;

typedef struct CpdTransferQueueFamily {
    CpdTransferQueue* queues;
    uint32_t queue_count;
    uint32_t index;
    VkCommandPool pool;
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
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;

    // == Semaphore

    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkDestroySemaphore vkDestroySemaphore;

    // == Fence

    PFN_vkCreateFence vkCreateFence;
    PFN_vkDestroyFence vkDestroyFence;
    PFN_vkResetFences vkResetFences;
    PFN_vkWaitForFences vkWaitForFences;

    // == Queue

    PFN_vkQueueWaitIdle vkQueueWaitIdle;
    PFN_vkQueueSubmit2KHR vkQueueSubmit2KHR;

    // == Image

    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkDestroyImageView vkDestroyImageView;

    // == Render Pass

    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkDestroyRenderPass vkDestroyRenderPass;

    // == Pipeline

    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkCreateComputePipelines vkCreateComputePipelines;
    PFN_vkDestroyPipeline vkDestroyPipeline;

    // == Framebuffer

    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer;

    // == Command Pool

    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkDestroyCommandPool vkDestroyCommandPool;
    PFN_vkResetCommandPool vkResetCommandPool;

    // == Dynamic Rendering

    PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
    PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;

    // == Command Buffer

    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers vkFreeCommandBuffers;

    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;

    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCmdClearColorImage vkCmdClearColorImage;
    PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR;
} CpdDevice;

void DEVICE_destroy(CpdDevice* cpeed_device);
VkResult DEVICE_initialize(
    CpdDevice* cpeed_device, VkPhysicalDevice physical, const CpdPlatformExtensions* extensions,
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t transfer_offset
);
VkResult DEVICE_reset_pools(CpdDevice* cpeed_device, VkCommandPoolResetFlags flags, bool reset_transfer);
VkResult DEVICE_wait_idle(CpdDevice* cpeed_device, bool wait_transfer);
