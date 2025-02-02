#pragma once

#include "renderer.h"

VkResult create_fence(CpdDevice* cpeed_device, VkFence* fence);

VkResult create_semaphore(CpdDevice* cpeed_device, VkSemaphore* semaphore);

VkResult create_primary_command_buffer(CpdDevice* cpeed_device, VkCommandPool pool, VkCommandBuffer* buffer);
