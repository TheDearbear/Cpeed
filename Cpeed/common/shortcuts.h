#pragma once

#include "renderer.h"

inline VkResult create_fence(CpdDevice* cpeed_device, VkFence* fence) {
    VkFenceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0
    };

    return cpeed_device->vkCreateFence(cpeed_device->handle, &create_info, VK_NULL_HANDLE, fence);
}

inline VkResult create_semaphore(CpdDevice* cpeed_device, VkSemaphore* semaphore) {
    VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0
    };

    return cpeed_device->vkCreateSemaphore(cpeed_device->handle, &create_info, VK_NULL_HANDLE, semaphore);
}

VkResult create_primary_command_buffer(CpdDevice* cpeed_device, VkCommandPool pool, VkCommandBuffer* buffer);
