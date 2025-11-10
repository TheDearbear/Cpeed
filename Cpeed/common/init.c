#include "frame_layers/test_layer.h"
#include "frame.h"

static CpdWindow g_window;
static uint32_t g_test_layer_handle;

void init_engine() {
    g_test_layer_handle = add_frame_layer(&g_frame_layer_functions_test, g_frame_layer_flags_test);
void init_engine(CpdWindow window) {
    g_window = window;
    g_test_layer_handle = add_frame_layer(window, &g_frame_layer_functions_test, g_frame_layer_flags_test);
}

void shutdown_engine() {
    remove_frame_layer(g_window, g_test_layer_handle);
}
