#pragma once

#include "platform/frame.h"
#include "platform/window.h"
#include "input.h"
#include "platform.h"

typedef void* CpdBackendHandle;

typedef struct CpdBackendVersion {
    uint16_t major;
    uint16_t minor;
} CpdBackendVersion;

typedef struct CpdBackendInfo {
    CpdWindow window;
    CpdVector3 background;
} CpdBackendInfo;

typedef struct CpdBackendImplementation {
    CpdPlatformBackendFlags type;

    bool (*initialize_backend)();
    void (*shutdown_backend)();
    CpdBackendHandle (*initialize_window)(const CpdBackendInfo* info);
    void (*shutdown_window)(CpdBackendHandle cpeed_backend);
    CpdBackendVersion (*get_version)(CpdBackendHandle cpeed_backend);
    CpdFrame* (*get_frame)(CpdBackendHandle cpeed_backend);
    bool (*resize)(CpdBackendHandle cpeed_backend, CpdSize new_size);
    bool (*should_frame)(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window);
    bool (*pre_frame)(CpdBackendHandle cpeed_backend);
    bool (*frame)(CpdBackendHandle cpeed_backend);
    bool (*post_frame)(CpdBackendHandle cpeed_backend);
} CpdBackendImplementation;

bool get_backend_implementation(CpdPlatformBackendFlags backend, CpdBackendImplementation* implementation);
