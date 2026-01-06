#include <io/pad.h>

#include <stdbool.h>

bool initialize_platform() {
    return ioPadInit(MAX_PORT_NUM) == 0;
}

void shutdown_platform() {
    ioPadEnd();
}
