#include <sys/systime.h>

#include <Cpeed/platform.h>

CpdCompilePlatform compile_platform() {
    return CpdCompilePlatform_PS3;
}

CpdPlatformBackendFlags platform_supported_backends() {
    return CpdPlatformBackendFlags_RSX;
}

uint64_t get_clock_usec() {
    u64 sec = 0, nsec = 0;
    
    if (sysGetCurrentTime(&sec, &nsec) != 0) {
        return 0;
    }

    return sec * 1000000 + nsec;
}
