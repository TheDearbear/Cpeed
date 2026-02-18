#include <roapi.h>

#include <Cpeed/platform/init.h>

#include "uwpmain.h"

bool initialize_platform() {
    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED))) {
        return false;
    }

    return QueryPerformanceFrequency(&g_counter_frequency) != FALSE;
}

void shutdown_platform() {
    RoUninitialize();
}
