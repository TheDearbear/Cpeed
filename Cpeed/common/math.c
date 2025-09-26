#include "math.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

uint32_t min_u32(uint32_t a, uint32_t b);
int32_t min_i32(int32_t a, int32_t b);

uint32_t max_u32(uint32_t a, uint32_t b);
int32_t max_i32(int32_t a, int32_t b);

uint32_t log2_u32(uint32_t a) {
    if (a == 0) {
        return UINT32_MAX;
    }

#if defined(_MSC_VER)
    uint32_t index = 0;
    _BitScanReverse(&index, a);
    return index;

#elif defined(__GNUC__)
    return (uint32_t)(31 ^ __builtin_clz(a));
#else
    static const uint32_t MultiplyDeBruijnBitPosition[32] =
    {
      0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
      8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;

    return MultiplyDeBruijnBitPosition[(uint32_t)(a * 0x07C4ACDDU) >> 27];
#endif
}
