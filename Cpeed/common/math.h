#pragma once

#include <stdint.h>

typedef struct CpdVector2 {
    float x;
    float y;
} CpdVector2;

typedef struct CpdVector3 {
    float x;
    float y;
    float z;
} CpdVector3;

typedef struct CpdVector4 {
    float x;
    float y;
    float z;
    float w;
} CpdVector4;

typedef struct CpdSize {
    unsigned short width;
    unsigned short height;
} CpdSize;

inline uint32_t min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

inline int32_t min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

inline uint32_t max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

inline int32_t max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

uint32_t log2_u32(uint32_t a);
