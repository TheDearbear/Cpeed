#include <malloc.h>

#include <Cpeed/platform/frame.h>
#include <Cpeed/platform/window.h>

#include "uwpmain.h"

CpdWindow create_window(const CpdWindowInfo* info) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)malloc(sizeof(CpdUWPWindow));
    if (uwp_window == 0) {
        return 0;
    }

    CpdInputEvent* input_queue = (CpdInputEvent*)malloc(sizeof(CpdInputEvent) * INPUT_QUEUE_BASE_SIZE);
    CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(sizeof(CpdInputEvent) * INPUT_QUEUE_BASE_SIZE);
    if (input_queue == 0 || input_swap_queue == 0) {
        free(input_queue);
        free(input_swap_queue);
        free(uwp_window);
        return 0;
    }

    uwp_window->core_window = 0;
    uwp_window->backend = 0;

    uwp_window->input_queue = input_queue;
    uwp_window->input_swap_queue = input_swap_queue;
    uwp_window->input_queue_size = 0;
    uwp_window->input_swap_queue_size = 0;
    uwp_window->input_queue_max_size = INPUT_QUEUE_BASE_SIZE;
    uwp_window->input_mode = info->input_mode;
    uwp_window->current_key_modifiers = CpdInputModifierKey_None;
    uwp_window->size = info->size;

    uwp_window->mouse_x = 0;
    uwp_window->mouse_y = 0;

    uwp_window->devices = 0;

    uwp_window->layers = 0;

    uwp_window->should_close = false;
    uwp_window->resized = false;
    uwp_window->visible = true;
    uwp_window->resize_swap_queue = false;
    uwp_window->first_mouse_event = true;

#ifdef CPD_IMGUI_AVAILABLE
    uwp_window->imgui_context = ImGui_CreateContext(0);
    ImGui_SetCurrentContext(uwp_window->imgui_context);
#endif

    return (CpdWindow)uwp_window;
}

static bool remove_frame_layers_loop(void* context, CpdFrameLayer* layer) {
    CpdWindow window = (CpdWindow)context;

    remove_frame_layer(window, layer->handle);

    return true;
}

void destroy_window(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    loop_frame_layers(window, remove_frame_layers_loop, (void*)window);

#ifdef CPD_IMGUI_AVAILABLE
    ImGui_DestroyContext(uwp_window->imgui_context);
#endif

    cleanup_input_queue(uwp_window->input_queue, uwp_window->input_queue_size);
    cleanup_input_queue(uwp_window->input_swap_queue, uwp_window->input_swap_queue_size);

    free(uwp_window->input_queue);
    free(uwp_window->input_swap_queue);
    free(uwp_window);
}

void close_window(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;
    
    uwp_window->should_close = true;
}

bool poll_window(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    poll_events(uwp_window);
    poll_gamepads(uwp_window);

    return uwp_window->should_close;
}

CpdSize window_size(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    return uwp_window->size;
}

bool window_resized(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    bool result = uwp_window->resized;
    uwp_window->resized = false;

    return result;
}

bool window_present_allowed(CpdWindow window) {
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    return uwp_window->visible;
}

ImGuiContext* get_imgui_context(CpdWindow window) {
#ifdef CPD_IMGUI_AVAILABLE
    CpdUWPWindow* uwp_window = (CpdUWPWindow*)window;

    return uwp_window->imgui_context;
#else
    return 0;
#endif
}

bool multiple_windows_supported() {
    return false;
}
