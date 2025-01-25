#pragma once

#define VK_NO_PROTOTYPES

#include <vulkan/vulkan.h>
#include <malloc.h>
#include <stdbool.h>

#include "../main.h"

extern void get_physical_device_families(VkPhysicalDevice physical,
    uint32_t* graphics, uint32_t* compute, uint32_t* transfer,
    uint32_t* transfer_count, uint32_t* transfer_offset);

extern bool allocate_queue_create_infos(
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t* count, VkDeviceQueueCreateInfo** infos);

extern void destroy_queue_create_infos(VkDeviceQueueCreateInfo* infos, uint32_t count);
