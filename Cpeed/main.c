#include "common/frame.h"
#include "common/init.h"
#include "platform/input/queue.h"
#include "platform/init.h"
#include "platform/logging.h"
#include "backend.h"
#include "platform.h"

#ifdef CPD_IMGUI_AVAILABLE
#include "common/imgui/imgui_impl_cpeed.h"
#include <dcimgui.h>
#endif

static bool get_lowest_frame_layer(void* context, CpdFrameLayer* frame_layer) {
    CpdFrameLayer** output = (CpdFrameLayer**)context;

    *output = frame_layer;

    return true;
}

int main() {
    if (!initialize_platform()) {
        log_error("Unable to initialize platform-specific information\n");
        return -1;
    }

    CpdBackendImplementation implementation;
    if (!get_backend_implementation(CpdPlatformBackendFlags_Vulkan, &implementation)) {
        log_error("Unable to find backend implementation\n");
        shutdown_platform();
        return -1;
    }

    if (!implementation.initialize_backend()) {
        log_error("Unable to initialize backend\n");
        shutdown_platform();
        return -1;
    }

#ifdef CPD_IMGUI_AVAILABLE
    log_debug("ImGui Version: %s\n", ImGui_GetVersion());
#endif

    CpdWindow window = create_window(&g_window_create_info);
    if (window == 0) {
        log_error("Unable to create window\n");
        implementation.shutdown_backend();
        shutdown_platform();
        return -1;
    }

#ifdef CPD_IMGUI_AVAILABLE
    if (!cImGui_ImplCpeed_Init(window)) {
        log_error("Unable to initialize ImGui for window\n");
        destroy_window(window);
        implementation.shutdown_backend();
        shutdown_platform();
        return -1;
    }
#endif

    CpdBackendInfo backend_info = {
        .window = window,
        .background.x = 0.2f,
        .background.y = 0.5f,
        .background.z = 0.5f
    };
    CpdBackendHandle backend = implementation.initialize_window(&backend_info);
    if (backend == 0) {
        log_error("Unable to initialize backend for window\n");
        destroy_window(window);
        implementation.shutdown_backend();
        shutdown_platform();
        return -1;
    }

    init_engine(window);

    CpdFrame* cpeed_frame = implementation.get_frame(backend);

    while (!poll_window(window)) {
        bool resized = window_resized(window);
        if (resized) {
            CpdSize size = window_size(window);

            if (!implementation.resize(backend, size)) {
                log_error("Unable to resize window\n");
                continue;
            }
        }

        const CpdInputEvent* input_events = 0;
        uint32_t input_event_count = 0;
        if (get_window_input_events(window, &input_events, &input_event_count)) {
            for (uint32_t i = 0; i < input_event_count; i++) {
                CpdFrameLayer* frame_layer = 0;
                loop_frame_layers(window, get_lowest_frame_layer, &frame_layer);

                while (frame_layer != 0) {
                    if (frame_layer->functions.input != 0 && !frame_layer->functions.input(window, cpeed_frame, &input_events[i])) {
                        break;
                    }

                    frame_layer = frame_layer->higher;
                }
            }
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

    log_info("Goodbye!\n");

    shutdown_engine();

#ifdef CPD_IMGUI_AVAILABLE
    cImGui_ImplCpeed_Shutdown();
#endif

    implementation.shutdown_window(backend);

    implementation.shutdown_backend();

    if (window != 0) {
        destroy_window(window);
    }

    shutdown_platform();
}
