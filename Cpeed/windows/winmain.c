#include "../platform.h"
#include "winmain.h"

IGameInput* g_game_input;
CpdGamepad* g_gamepads;
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

bool resize_input_queue_if_need(WindowExtraData* data, uint32_t new_events) {
    uint32_t new_size = data->input_queue_size + new_events;

    if (data->input_queue_max_size >= new_size) {
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

    for (uint32_t i = 0; i < data->input_queue_size; i++) {
        new_input_queue[i] = data->input_queue[i];
    }

    CpdInputEvent* old_input_queue = data->input_queue;

    data->input_queue = new_input_queue;
    data->input_queue_max_size = new_max_size;
    data->resize_swap_queue = true;

    free(old_input_queue);

    return true;
}
