#include "../platform.h"
#include "winmain.h"

LARGE_INTEGER g_counter_frequency;
ATOM g_window_class;

CpdCompilePlatform compile_platform() {
    return CpdCompilePlatform_Windows;
}

CpdPlatformBackendFlags platform_supported_backends() {
    return CpdPlatformBackendFlags_Vulkan;
}

uint64_t get_clock_usec() {
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    return counter.QuadPart * 1000000 / g_counter_frequency.QuadPart;
}

void cleanup_input_queue(CpdInputEvent* queue, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        CpdInputEvent* event = &queue[i];

        if (event->type == CpdInputEventType_TextInput) {
            free(event->data.text_input.text);
            event->data.text_input.text = 0;
        }
    }
}
