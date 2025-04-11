#pragma once

#include "general.h"
#include "device.h"

typedef struct CpdImage {
    VkImage handle;
    VkImageView view;
    VkPipelineStageFlags2KHR stage;
    VkAccessFlags2 access;
    VkImageLayout layout;
    VkFormat format;
    uint32_t queue_family_index;
    CpdSize size;
} CpdImage;
