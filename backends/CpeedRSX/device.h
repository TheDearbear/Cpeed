#pragma once

#include <ppu-types.h>

#include <Cpeed/platform/frame.h>

#define HOST_SIZE (1 * 1024 * 1024) // 1MB
#define CB_SIZE (64 * 1024) // 64KB

#define COLOR_BUFFER_NUM 2

typedef struct CpdOutputBuffers {
    void* color;
    void* depth;
} CpdOutputBuffers;

typedef struct CpdOutputBufferOffsets {
    u32 color;
    u32 depth;
} CpdOutputBufferOffsets;

typedef struct CpdOutputDevice {
    u32 video_out;
    u32 device_index;

    CpdSize size;
    u32 frame_index;

    CpdOutputBuffers base_address;
    CpdOutputBuffers addresses[COLOR_BUFFER_NUM];
    CpdOutputBufferOffsets offsets[COLOR_BUFFER_NUM];

    u32 color_pitch;
    u32 depth_pitch;

    CpdFrame* frame;
} CpdOutputDevice;
