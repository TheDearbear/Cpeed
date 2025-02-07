#pragma once

#include "../vulkan.h"

typedef struct CpdSize {
    unsigned short width;
    unsigned short height;
} CpdSize;

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

uint32_t min_u32(uint32_t a, uint32_t b);
uint32_t max_u32(uint32_t a, uint32_t b);
