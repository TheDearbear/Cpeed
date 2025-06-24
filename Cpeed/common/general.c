#include "general.h"

uint32_t min_u32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

int32_t min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

uint32_t max_u32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

int32_t max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}
