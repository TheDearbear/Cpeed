#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>
#include <windows.h>

static void log_raw(const char* format, va_list list) {
    int bytes = vsnprintf(0, 0, format, list);
    char* buffer = (char*)malloc(bytes + 1);

    if (buffer != 0) {
        buffer[0] = 0;
        vsnprintf(buffer, bytes + 1, format, list);

        OutputDebugStringA(buffer);
    }

    free(buffer);
}

void log_debug(const char* format, ...) {
    va_list list;
    va_start(list, format);
    log_raw(format, list);
    va_end(list);
}

void log_info(const char* format, ...) {
    va_list list;
    va_start(list, format);
    log_raw(format, list);
    va_end(list);
}

void log_warning(const char* format, ...) {
    va_list list;
    va_start(list, format);
    log_raw(format, list);
    va_end(list);
}

void log_error(const char* format, ...) {
    va_list list;
    va_start(list, format);
    log_raw(format, list);
    va_end(list);
}
