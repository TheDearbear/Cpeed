#include "winmain.h"
#include <stdio.h>

static int windows_created = 0;
static ATOM window_class = 0;

static void register_class();
static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define GET_EXTRA_DATA(hWnd) ((WindowExtraData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA))

CpdWindow PLATFORM_create_window(const CpdWindowInfo* info) {
    if (window_class == 0) {
        register_class();
    }

    HWND hWnd = CreateWindowExA(0, MAKEINTATOM(window_class), info->title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, info->size.width, info->size.height,
        NULL, NULL, GetModuleHandleW(NULL), NULL);
    
    if (hWnd == 0) {
        return 0;
    }

    WindowExtraData* data = (WindowExtraData*)malloc(sizeof(WindowExtraData));
    if (data == 0) {
        return 0;
    }

    data->resized = false;
    data->should_close = false;
    data->minimized = false;
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)data);

    windows_created++;
    return (CpdWindow)hWnd;
}

void PLATFORM_window_destroy(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);
    free(data);

    if (DestroyWindow((HWND)window) && --windows_created == 0) {
        UnregisterClassW((LPCWSTR)window_class, NULL);
        window_class = 0;
    }
}

void PLATFORM_window_close(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    data->should_close = true;
}

bool PLATFORM_window_poll(CpdWindow window) {
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

CpdSize PLATFORM_get_window_size(CpdWindow window) {
    RECT rectangle = { 0, 0, 0, 0 };
    GetClientRect((HWND)window, &rectangle);
    
    return (CpdSize){
        .width = (unsigned short)(rectangle.right - rectangle.left),
        .height = (unsigned short)(rectangle.bottom - rectangle.top)
    };
}

bool PLATFORM_window_resized(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);
    
    bool resized = data->resized;
    data->resized = false;

    return resized;
}

bool PLATFORM_window_present_allowed(CpdWindow window) {
    WindowExtraData* data = GET_EXTRA_DATA((HWND)window);

    return !data->minimized;
}

static void register_class() {
    if (window_class != 0) {
        return;
    }

    WNDCLASSEXW wndClassExW = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = wndProc,
        .hInstance = GetModuleHandleW(NULL),
        .hCursor = LoadImageW(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
        .lpszClassName = L"Cpeed"
    };

    window_class = RegisterClassExW(&wndClassExW);
}

static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SYSCOMMAND:
            switch (GET_SC_WPARAM(wParam)) {
                case SC_CLOSE:
                    goto wndProc_close;

                case SC_MAXIMIZE:
                case SC_RESTORE:
                    DefWindowProcW(hWnd, uMsg, wParam, lParam);
                    goto wndProc_sizing;
            }
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        case WM_SIZE:
            {
                WindowExtraData* data = GET_EXTRA_DATA(hWnd);
                if (data != 0) {
                    data->minimized = wParam & SIZE_MINIMIZED != 0;
                }
            }
            return 0;

        case WM_CLOSE:
        wndProc_close:
            PLATFORM_window_close((CpdWindow)hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZING:
        wndProc_sizing:
            {
                WindowExtraData* data = GET_EXTRA_DATA(hWnd);

                data->resized = true;
            }
            return 0;

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
