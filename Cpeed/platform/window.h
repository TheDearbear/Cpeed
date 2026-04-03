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

typedef enum CpdWindowFlags {
    CpdWindowFlags_None
} CpdWindowFlags;

typedef struct CpdWindowInfo {
    const char* title;
    CpdSize size;
    CpdWindowFlags flags;
    CpdInputMode input_mode;
} CpdWindowInfo;

/* Creates new window handle (and creates actual window if supported by platform) */
extern CpdWindow create_window(const CpdWindowInfo* info);

/* Closes and frees any resource for provided window */
extern void destroy_window(CpdWindow window);

/* Programmic way to close window */
extern void close_window(CpdWindow window);

/* Checks for any updates for provided window */
extern bool poll_window(CpdWindow window);

/* Returns current pixel size of window */
extern CpdSize window_size(CpdWindow window);
extern bool window_resized(CpdWindow window);

/* Checks if there is any reason to present image to the window */
extern bool window_present_allowed(CpdWindow window);

/* Returns pointer to ImGui context if enabled. (zero if not) */
extern ImGuiContext* get_imgui_context(CpdWindow window);

/* Returns true if more than one window supported by platform */
extern bool multiple_windows_supported();

/* Returns true if windowed is supported by platform */
extern bool windowed_mode_supported();
