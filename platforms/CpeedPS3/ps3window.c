#include <malloc.h>

#include <Cpeed/platform/input/gamepad.h>
#include <Cpeed/platform/frame.h>
#include <Cpeed/platform/window.h>
#include <Cpeed/platform/logging.h>
#include <Cpeed/platform.h>

#include "ps3main.h"

CpdWindow create_window(const CpdWindowInfo* info) {
    
    CpdPs3Window* ps3_window = (CpdPs3Window*)malloc(sizeof(CpdPs3Window));
    //CpdInputEvent* input_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    //CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    //CpdKeyboardKey* keyboard_presses = (CpdKeyboardKey*)malloc(KEYBOARD_PRESSES_BASE_SIZE * sizeof(CpdKeyboardKey));

    if (ps3_window == 0) { //|| input_queue == 0 || input_swap_queue == 0 || keyboard_presses == 0) {
        //free(ps3_window);
        //free(input_queue);
        //free(input_swap_queue);
        //free(keyboard_presses);
        return 0;
    }

#ifdef CPD_IMGUI_AVAILABLE
    ps3_window->imgui_context = ImGui_CreateContext(0);
    ImGui_SetCurrentContext(ps3_window->imgui_context);
#endif

    /*data->input_queue = input_queue;
    data->input_swap_queue = input_swap_queue;
    data->input_queue_size = 0;
    data->input_swap_queue_size = 0;
    data->input_queue_max_size = INPUT_QUEUE_BASE_SIZE;
    data->input_mode = info->input_mode;
    data->current_key_modifiers = CpdInputModifierKey_None;

    RECT rectangle = { 0, 0, 0, 0 };
    GetClientRect(hWnd, &rectangle);

    data->size = (CpdSize){
        .width = (unsigned short)(rectangle.right - rectangle.left),
        .height = (unsigned short)(rectangle.bottom - rectangle.top),
    };
    data->mouse_x = 0;
    data->mouse_y = 0;

    data->keyboard_presses = keyboard_presses;
    data->keyboard_presses_size = 0;
    data->keyboard_presses_max_size = KEYBOARD_PRESSES_BASE_SIZE;*/

    ps3_window->layers = 0;

    ps3_window->should_close = false;
    ps3_window->resized = false;
    ps3_window->focused = true;

    /*data->left_shift_pressed = false;
    data->right_shift_pressed = false;
    data->left_control_pressed = false;
    data->right_control_pressed = false;
    data->left_alt_pressed = false;
    data->right_alt_pressed = false;

    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)data);

    uint16_t count = get_gamepad_count((CpdWindow)hWnd);
    if (!resize_input_queue_if_need(data, count)) {
        destroy_window((CpdWindow)hWnd);
        return 0;
    }

    for (uint16_t i = 0; i < count; i++) {
        data->input_queue[data->input_queue_size++] = (CpdInputEvent){
            .type = CpdInputEventType_GamepadConnect,
            .modifiers = data->current_key_modifiers,
            .time = get_clock_usec(),
            .data.gamepad_connect.status = CpdGamepadConnectStatus_Connected,
            .data.gamepad_connect.gamepad_index = i
        };
    }

    if (data->use_raw_input) {
        data->use_raw_input = register_raw_input(hWnd);
    }*/

    return (CpdWindow)ps3_window;
}

static bool remove_frame_layers_loop(void* context, CpdFrameLayer* layer) {
    CpdWindow window = (CpdWindow)context;

    remove_frame_layer(window, layer->handle);

    return true;
}

void destroy_window(CpdWindow window) {
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    loop_frame_layers(window, remove_frame_layers_loop, (void*)window);

#ifdef CPD_IMGUI_AVAILABLE
    ImGui_DestroyContext(ps3_window->imgui_context);
#endif

    /*cleanup_input_queue(data->input_queue, data->input_queue_size);
    cleanup_input_queue(data->input_swap_queue, data->input_swap_queue_size);

    free(data->input_queue);
    free(data->input_swap_queue);
    free(data->keyboard_presses);*/
    free(ps3_window);

    //DestroyWindow((HWND)window);
}

void close_window(CpdWindow window) {
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    ps3_window->should_close = true;
}

bool poll_window(CpdWindow window) {
    //MSG msg;
    //while (PeekMessageW(&msg, (HWND)window, 0, 0, PM_REMOVE)) {
    //    TranslateMessage(&msg);
    //    DispatchMessageW(&msg);
    //}

    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    //if (data == 0) {
    //    DWORD err = GetLastError();
    //    log_error("Unable to read window state. Error code: %d\n", err);
    //    return true;
    //}

    return ps3_window->should_close;
}

CpdSize window_size(CpdWindow window) {
    //WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return (CpdSize) { .width = 0, .height = 0 };//return data->size;
}

bool window_resized(CpdWindow window) {
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    bool resized = ps3_window->resized;
    ps3_window->resized = false;

    return resized;
}

bool window_present_allowed(CpdWindow window) {
    //WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return true;//return !data->minimized;
}

ImGuiContext* get_imgui_context(CpdWindow window) {
#ifdef CPD_IMGUI_AVAILABLE
    CpdPs3Window* ps3_window = (CpdPs3Window*)window;

    return ps3_window->imgui_context;
#else
    return 0;
#endif
}

bool multiple_windows_supported() {
    return false;
}

bool windowed_mode_supported() {
    return false;
}
