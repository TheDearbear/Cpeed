#pragma once

#ifdef CPD_IMGUI_AVAILABLE

#include <dcimgui.h>

#include "../../platform/window.h"

CIMGUI_IMPL_API bool cImGui_ImplCpeed_Init(CpdWindow window);
CIMGUI_IMPL_API void cImGui_ImplCpeed_Shutdown(void);
CIMGUI_IMPL_API void cImGui_ImplCpeed_NewFrame(void);

#endif
