#pragma once

#include "../vulkan.h"

typedef struct CpdSize {
    unsigned short width;
    unsigned short height;
} CpdSize;

uint32_t min_u32(uint32_t a, uint32_t b);
uint32_t max_u32(uint32_t a, uint32_t b);
