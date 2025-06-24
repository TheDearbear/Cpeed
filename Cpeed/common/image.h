#pragma once

#include "general.h"
#include "device.h"

typedef struct CpdImageMemory {
    VkDeviceMemory handle;
    VkDeviceSize memory_offset;
    VkDeviceSize memory_align;
    VkDeviceSize image_size;
} CpdImageMemory;

typedef struct CpdImage {
    VkImage handle;
    VkImageView view;
    VkPipelineStageFlags2KHR stage;
    VkAccessFlags2 access;
    VkImageLayout layout;
    VkFormat format;
    uint32_t queue_family_index;
    CpdSize size;
    CpdImageMemory memory;
} CpdImage;

CpdImage* IMAGE_create(CpdDevice* cpeed_device, VkImageCreateInfo* create_info);
void IMAGE_destroy(CpdImage* cpeed_image, CpdDevice* cpeed_device);
