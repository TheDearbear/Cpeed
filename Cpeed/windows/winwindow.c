#include <malloc.h>
#include <stdio.h>

#include "../platform/window.h"
#include "winmain.h"

CpdWindow create_window(const CpdWindowInfo* info) {
    WindowExtraData* data = (WindowExtraData*)malloc(sizeof(WindowExtraData));
    CpdInputEvent* input_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));
    CpdInputEvent* input_swap_queue = (CpdInputEvent*)malloc(INPUT_QUEUE_BASE_SIZE * sizeof(CpdInputEvent));

    if (data == 0 || input_queue == 0 || input_swap_queue == 0) {
        free(data);
        free(input_queue);
        free(input_swap_queue);
        return 0;
    }

    HWND hWnd = CreateWindowExA(0, MAKEINTATOM(g_window_class), info->title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, info->size.width, info->size.height,
        NULL, NULL, GetModuleHandleW(NULL), NULL);
    
    if (hWnd == 0) {
        free(data);
        free(input_queue);
        free(input_swap_queue);
        return 0;
    }

    data->input_queue = input_queue;
    data->input_swap_queue = input_swap_queue;
    data->input_queue_size = 0;
    data->input_swap_queue_size = 0;
    data->input_queue_max_size = INPUT_QUEUE_BASE_SIZE;
    data->input_mode = info->input_mode;
    data->current_key_modifiers = CpdInputModifierKey_None;

    data->mouse_x = 0;
    data->mouse_y = 0;

    data->resized = false;
    data->should_close = false;
    data->minimized = false;
    data->resize_swap_queue = false;
    data->first_mouse_event = true;
    
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)data);

    return (CpdWindow)hWnd;
}

void destroy_window(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    cleanup_input_queue(data->input_queue, data->input_queue_size);
    cleanup_input_queue(data->input_swap_queue, data->input_swap_queue_size);

    free(data->input_queue);
    free(data->input_swap_queue);
    free(data);

    DestroyWindow((HWND)window);
}

void close_window(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    data->should_close = true;
}

bool poll_window(CpdWindow window) {
    MSG msg;
    while (PeekMessageW(&msg, (HWND)window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    if (data == 0) {
        DWORD err = GetLastError();
        printf("Unable to read window state. Error code: %d\n", err);
        return true;
    }

    return data->should_close;
}

CpdSize window_size(CpdWindow window) {
    RECT rectangle = { 0, 0, 0, 0 };
    GetClientRect((HWND)window, &rectangle);
    
    return (CpdSize){
        .width = (unsigned short)(rectangle.right - rectangle.left),
        .height = (unsigned short)(rectangle.bottom - rectangle.top)
    };
}

bool window_resized(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);
    
    bool resized = data->resized;
    data->resized = false;

    return resized;
}

bool window_present_allowed(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return !data->minimized;
}

bool multiple_windows_supported() {
    return true;
}
