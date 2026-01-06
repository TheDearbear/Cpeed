#pragma once

#include <stdint.h>

typedef enum CpdCompilePlatform {
    CpdCompilePlatform_Windows,
    CpdCompilePlatform_UWP,
    CpdCompilePlatform_Linux,
    CpdCompilePlatform_PS3
} CpdCompilePlatform;

typedef enum CpdPlatformBackendFlags {
    CpdPlatformBackendFlags_Vulkan = 1 << 0,
    CpdPlatformBackendFlags_DirectX = 1 << 1,
    CpdPlatformBackendFlags_RSX = 1 << 2
} CpdPlatformBackendFlags;

extern CpdCompilePlatform compile_platform();

extern CpdPlatformBackendFlags platform_supported_backends();

extern uint64_t get_clock_usec();
