#include "winmain.h"
#include <malloc.h>
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
        CW_USEDEFAULT, CW_USEDEFAULT, info->width, info->height,
        NULL, NULL, GetModuleHandleW(NULL), NULL);

    free(str);

    if (hWnd != 0) {
        windows_created++;
        return (CpdWindow)hWnd;
    }

    return 0;
}

void PLATFORM_window_show(CpdWindow window) {
    ShowWindow((HWND)window, 1);
}

void PLATFORM_window_hide(CpdWindow window) {
    ShowWindow((HWND)window, 0);
}

void PLATFORM_window_destroy(CpdWindow window) {
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

    LONG should_close = GetWindowLongW((HWND)window, 0);

    DWORD err = GetLastError();
    if (err != 0) {
        printf("Unable to read window state. Error code: %d\n", err);
        return true;
    }

    return should_close;
}

static void register_class() {
    if (window_class != NULL) {
        return;
    }

    WNDCLASSEXW wndClassExW = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = wndProc,
        .cbWndExtra = sizeof(LONG),
        .hInstance = GetModuleHandleW(NULL),
        .lpszClassName = L"Cpeed"
    };

    window_class = RegisterClassExW(&wndClassExW);
}

static void close_window(HWND hWnd) {
    SetWindowLongW(hWnd, 0, true); // should_close = true
}

static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SYSCOMMAND:
            if ((wParam & 0xFFF0) == SC_CLOSE) {
                close_window(hWnd);
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

        case WM_CREATE:
        case WM_SIZE:
            return 0;

        default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
