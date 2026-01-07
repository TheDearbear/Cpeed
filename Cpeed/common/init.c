#include "../platform/frame.h"
#include "frame_layers/test_layer.h"

static CpdWindow g_window;
static uint32_t g_test_layer_handle;

CpdWindowInfo g_window_create_info = {
    .title = "Cpeed",
    .size.width = 800,
    .size.height = 600,
    .flags = CpdWindowFlags_None,
    .input_mode = CpdInputMode_KeyCode
};

void init_engine(CpdWindow window) {
    g_window = window;
    g_test_layer_handle = add_frame_layer(window, &g_frame_layer_functions_test, 0, g_frame_layer_flags_test);
}

void shutdown_engine() {
    remove_frame_layer(g_window, g_test_layer_handle);
}
