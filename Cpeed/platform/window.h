#pragma once

#if CPD_IMGUI_AVAILABLE
#include <dcimgui.h>
#else
typedef struct ImGuiContext_t ImGuiContext;
#endif

#include <stdbool.h>

#include "../common/math.h"
#include "../input/keyboard.h"

typedef void* CpdWindow;

// Reserved for future use
typedef unsigned int CpdWindowFlags;

typedef struct CpdWindowInfo {
    const char* title;
    CpdSize size;
    CpdWindowFlags flags;
    CpdInputMode input_mode;
} CpdWindowInfo;

extern CpdWindow create_window(const CpdWindowInfo* info);
extern void destroy_window(CpdWindow window);

extern void close_window(CpdWindow window);

extern bool poll_window(CpdWindow window);

extern CpdSize window_size(CpdWindow window);
extern bool window_resized(CpdWindow window);

extern bool window_present_allowed(CpdWindow window);

extern ImGuiContext* get_imgui_context(CpdWindow window);

extern bool multiple_windows_supported();

extern bool windowed_mode_supported();
