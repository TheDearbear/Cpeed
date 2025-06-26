#include "winmain.h"
#include <stdio.h>

static int windows_created;
static ATOM window_class;

static void register_class();
static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CpdWindow PLATFORM_create_window(const CpdWindowInfo* info) {
    size_t len = strlen(info->title);
    char* str = (char*)malloc((len + 1) * sizeof(char));
    if (str == 0) {
        return 0;
    }

    strcpy_s(str, len + 1, info->title);

    if (window_class == NULL) {
        register_class();
    }

    HWND hWnd = CreateWindowExA(0, window_class, str, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, info->size.width, info->size.height,
        NULL, NULL, GetModuleHandleW(NULL), NULL);

    free(str);
    
    if (hWnd == 0) {
        return 0;
    }

    WindowExtraData* data = (WindowExtraData*)malloc(sizeof(WindowExtraData));
    if (data == 0) {
        return 0;
    }

    data->resized = false;
    data->should_close = false;
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, data);

    windows_created++;
    return (CpdWindow)hWnd;
}

void PLATFORM_window_destroy(CpdWindow window) {
    WindowExtraData* data = (WindowExtraData*)GetWindowLongPtrW((HWND)window, GWLP_USERDATA);
    free(data);

    if (DestroyWindow((HWND)window) && --windows_created == 0) {
        UnregisterClassW(window_class, NULL);
        window_class = NULL;
    }
}

bool PLATFORM_window_poll(CpdWindow window) {
    MSG msg;
    while (PeekMessageW(&msg, (HWND)window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    WindowExtraData* data = (WindowExtraData*)GetWindowLongPtrW((HWND)window, GWLP_USERDATA);

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
        .width = rectangle.right - rectangle.left,
        .height = rectangle.bottom - rectangle.top
    };
}

bool PLATFORM_window_resized(CpdWindow window) {
    WindowExtraData* data = (WindowExtraData*)GetWindowLongPtrW((HWND)window, GWLP_USERDATA);
    
    bool resized = data->resized;
    data->resized = false;

    return resized;
}

static void register_class() {
    if (window_class != NULL) {
        return;
    }

    WNDCLASSEXW wndClassExW = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = wndProc,
        .hInstance = GetModuleHandleW(NULL),
        .lpszClassName = L"Cpeed"
    };

    window_class = RegisterClassExW(&wndClassExW);
}

static void close_window(HWND hWnd) {
    ((WindowExtraData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA))->should_close = true;
}

static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SYSCOMMAND:
            switch (wParam & 0xFFF0) {
                case SC_CLOSE:
                    close_window(hWnd);
                    return 0;

                case SC_MAXIMIZE:
                case SC_RESTORE:
                    {
                        DefWindowProcW(hWnd, uMsg, wParam, lParam);
                        ((WindowExtraData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA))->resized = true;
                    }
                    return 0;
            }
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);

        case WM_CLOSE:
            close_window(hWnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                EndPaint(hWnd, &ps);
            }
            return 0;

        case WM_SIZING:
            ((WindowExtraData*)GetWindowLongW(hWnd, GWLP_USERDATA))->resized = true;
            return 0;

        case WM_CREATE:
            return 0;

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
