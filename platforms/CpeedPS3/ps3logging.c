#include <stdarg.h>
#include <stdio.h>

void log_debug(const char* format, ...) {
    va_list list;
    va_start(list, format);

    vfprintf(stdout, format, list);

    va_end(list);
}

void log_info(const char* format, ...) {
    va_list list;
    va_start(list, format);

    vfprintf(stdout, format, list);

    va_end(list);
}

void log_warning(const char* format, ...) {
    va_list list;
    va_start(list, format);

    vfprintf(stdout, format, list);

    va_end(list);
}

void log_error(const char* format, ...) {
    va_list list;
    va_start(list, format);

    vfprintf(stderr, format, list);

    va_end(list);
}
