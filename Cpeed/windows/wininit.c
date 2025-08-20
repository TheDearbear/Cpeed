#include <stdbool.h>

#include "winmain.h"
#include "winproc.h"

bool initialize_platform() {
    if (QueryPerformanceFrequency(&g_counter_frequency) == 0) {
        return false;
    }

    WNDCLASSEXW wndClassExW = {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = window_procedure,
        .hInstance = GetModuleHandleW(NULL),
        .hCursor = LoadImageW(NULL, (LPCWSTR)IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED),
        .lpszClassName = L"Cpeed"
    };

    g_window_class = RegisterClassExW(&wndClassExW);

    return g_window_class != 0;
}

void shutdown_platform() {
    UnregisterClassW((LPCWSTR)g_window_class, NULL);
    g_window_class = 0;
}
