#include "../platform/frame.h"
#include "frame_layers/test_layer.h"

typedef struct CpdFrameLayerInstance {
    const CpdFrameLayerFunctions* functions;
    const CpdFrameLayerFlags* flags;
    const void* context;
    uint32_t handle;
} CpdFrameLayerInstance;

static CpdWindow g_window;

static CpdFrameLayerInstance g_frame_layer_instances[] = {
    {
        .functions = &g_frame_layer_functions_test,
        .flags = &g_frame_layer_flags_test,
        .context = 0
    }
};

CpdWindowInfo g_window_create_info = {
    .title = "Cpeed",
    .size.width = 800,
    .size.height = 600,
    .flags = CpdWindowFlags_None,
    .input_mode = CpdInputMode_KeyCode
};

void init_engine(CpdWindow window) {
    g_window = window;

    for (size_t i = 0; i < sizeof(g_frame_layer_instances) / sizeof(CpdFrameLayerInstance); i++) {
        CpdFrameLayerInstance* instance = &g_frame_layer_instances[i];

        instance->handle = add_frame_layer(window, instance->functions, instance->context, *instance->flags);
    }
}

void shutdown_engine() {
    for (size_t i = 0; i < sizeof(g_frame_layer_instances) / sizeof(CpdFrameLayerInstance); i++) {
        CpdFrameLayerInstance* instance = &g_frame_layer_instances[i];

        remove_frame_layer(g_window, instance->handle);
    }
}
