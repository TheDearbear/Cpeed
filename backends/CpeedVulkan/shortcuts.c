#include "shortcuts.h"

double get_delta_time(CpdRenderer* cpeed_renderer, uint64_t current_time);

VkResult create_fence(CpdDevice* cpeed_device, VkFence* fence);

VkResult create_semaphore(CpdDevice* cpeed_device, VkSemaphore* semaphore);

VkResult create_primary_command_buffer(CpdDevice* cpeed_device, VkCommandPool pool, VkCommandBuffer* buffer) {
    VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    return cpeed_device->vkAllocateCommandBuffers(cpeed_device->handle, &allocate_info, buffer);
}
