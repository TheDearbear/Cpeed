#pragma once

#include <stdint.h>

typedef struct CpdSize {
    unsigned short width;
    unsigned short height;
} CpdSize;

uint32_t min_u32(uint32_t a, uint32_t b);
int32_t min_i32(int32_t a, int32_t b);

uint32_t max_u32(uint32_t a, uint32_t b);
int32_t max_i32(int32_t a, int32_t b);

uint32_t log2_u32(uint32_t a);
