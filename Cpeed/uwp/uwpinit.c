#include "../platform/init.h"
#include "uwpmain.h"

bool initialize_platform() {
    return QueryPerformanceFrequency(&g_counter_frequency) != 0;
}

void shutdown_platform() { }
