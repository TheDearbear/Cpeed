#pragma once

#include <stdbool.h>

#include "platform/input.h"
#include "platform/window.h"
#include "platform.h"

typedef void* CpdBackendHandle;

typedef struct CpdBackendVersion {
    uint16_t major;
    uint16_t minor;
} CpdBackendVersion;

typedef struct CpdBackendImplementation {
    CpdPlatformBackendFlags type;

    bool (*initialize_backend)();
    void (*shutdown_backend)();
    CpdBackendHandle (*initialize_window)(CpdWindow cpeed_window);
    void (*shutdown_window)(CpdBackendHandle cpeed_backend);
    CpdBackendVersion (*get_version)(CpdBackendHandle cpeed_backend);
    bool (*resize)(CpdBackendHandle cpeed_backend, CpdSize new_size);
    void (*input)(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window, const CpdInputEvent* input_events, uint32_t input_event_count);
    bool (*should_frame)(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window);
    bool (*pre_frame)(CpdBackendHandle cpeed_backend);
    bool (*frame)(CpdBackendHandle cpeed_backend);
    bool (*post_frame)(CpdBackendHandle cpeed_backend);
} CpdBackendImplementation;

bool get_backend_implementation(CpdPlatformBackendFlags backend, CpdBackendImplementation* implementation);
