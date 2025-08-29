#pragma once

#include <malloc.h>

#include "vulkan.h"

extern VkResult allocate_queue_create_infos(
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t* count, VkDeviceQueueCreateInfo** infos);

extern void destroy_queue_create_infos(VkDeviceQueueCreateInfo* infos, uint32_t count);
