#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>

#include "backend.h"
#include "platform.h"
#include "platform/init.h"
#include "platform/input.h"

int main() {
    if (!initialize_platform()) {
        printf("Unable to initialize platform-specific information\n");
        return -1;
    }

    CpdBackendImplementation implementation;
    if (!get_backend_implementation(CpdPlatformBackendFlags_Vulkan, &implementation)) {
        printf("Unable to find backend implementation\n");
        shutdown_platform();
        return -1;
    }

    if (!implementation.initialize_backend()) {
        printf("Unable to initialize backend\n");
        shutdown_platform();
        return -1;
    }

    CpdWindowInfo windowInfo = {
        .title = "Cpeed",
        .size.width = 800,
        .size.height = 600,
        .input_mode = CpdInputMode_KeyCode
    };
    CpdWindow window = create_window(&windowInfo);

    CpdBackendHandle backend = implementation.initialize_window(window);
    if (backend == 0) {
        printf("Unable to initialize backend for window\n");
        implementation.shutdown_backend();
        shutdown_platform();
        return -1;
    }

    while (!poll_window(window)) {
        bool resized = window_resized(window);
        if (resized) {
            CpdSize size = window_size(window);

            if (!implementation.resize(backend, size)) {
                printf("Unable to resize window\n");
                continue;
            }
        }

        const CpdInputEvent* input_events = 0;
        uint32_t input_event_count = 0;
        if (get_window_input_events(window, &input_events, &input_event_count)) {
            implementation.input(backend, window, input_events, input_event_count);
        }

        if (!implementation.should_frame(backend, window)) {
            continue;
        }

        if (!implementation.pre_frame(backend)) {
            break;
        }

        implementation.frame(backend);

        implementation.post_frame(backend);
    }

    printf("Goodbye!\n");

shutdown:
    implementation.shutdown_window(backend);

    implementation.shutdown_backend();

    if (window != 0) {
        destroy_window(window);
    }

    shutdown_platform();
}
