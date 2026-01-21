#include <malloc.h>
#include <time.h>

#include <Cpeed/platform.h>

#include "linuxmain.h"

CpdCompilePlatform compile_platform() {
    return CpdCompilePlatform_Linux;
}

const char* compile_platform_name() {
    return "Linux";
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

bool resize_input_queue_if_need(CpdWaylandWindow* window, uint32_t new_events) {
    uint32_t new_size = window->input_queue_size + new_events;

    if (window->input_queue_max_size >= new_size) {
        return true;
    }

    uint32_t new_max_size = new_size;
    uint32_t remainder = new_max_size % INPUT_QUEUE_SIZE_STEP;

    if (remainder != 0) {
        new_max_size += INPUT_QUEUE_SIZE_STEP - remainder;
    }

    CpdInputEvent* new_input_queue = (CpdInputEvent*)malloc(new_max_size * sizeof(CpdInputEvent));
    if (new_input_queue == 0) {
        return false;
    }

    for (uint32_t i = 0; i < window->input_queue_size; i++) {
        new_input_queue[i] = window->input_queue[i];
    }

    CpdInputEvent* old_input_queue = window->input_queue;

    window->input_queue = new_input_queue;
    window->input_queue_max_size = new_max_size;
    window->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}
