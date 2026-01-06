#pragma once

#include <Cpeed/platform/window.h>

typedef struct CpdPs3Window {
    struct CpdFrameLayer* layers;

    ImGuiContext* imgui_context;

    uint32_t should_close : 1;
    uint32_t resized : 1;
    uint32_t focused : 1;
} CpdPs3Window;
