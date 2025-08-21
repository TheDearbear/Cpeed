#include <malloc.h>
#include <time.h>

#include "../platform.h"
#include "../input.h"

CpdCompilePlatform compile_platform() {
    return CpdCompilePlatform_Linux;
}

CpdPlatformBackendFlags platform_supported_backends() {
    return CpdPlatformBackendFlags_Vulkan;
}

uint64_t get_clock_usec() {
    struct timespec clock;
    if (clock_gettime(CLOCK_MONOTONIC, &clock) != 0) {
        return 0;
    }

    return ((uint64_t)clock.tv_sec * 1000000) + (clock.tv_nsec / 1000);
}

void cleanup_input_queue(CpdInputEvent* events, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        CpdInputEvent* event = &events[i];

        if (event->type == CpdInputEventType_TextInput) {
            free(event->data.text_input.text);
            event->data.text_input.text = 0;
        }
    }
}
