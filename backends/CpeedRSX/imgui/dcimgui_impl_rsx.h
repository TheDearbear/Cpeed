#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <dcimgui.h>
#include <rsx/rsx.h>

#ifndef IMGUI_DISABLE

typedef struct ImDrawData_t ImDrawData;

CIMGUI_IMPL_API bool cImGui_ImplRSX_Init(gcmContextData* ctx);
CIMGUI_IMPL_API void cImGui_ImplRSX_Shutdown(void);
CIMGUI_IMPL_API void cImGui_ImplRSX_NewFrame(void);
CIMGUI_IMPL_API void cImGui_ImplRSX_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
CIMGUI_IMPL_API bool cImGui_ImplRSX_CreateDeviceObjects(void);
CIMGUI_IMPL_API void cImGui_ImplRSX_InvalidateDeviceObjects(void);

// (Advanced) Use e.g. if you need to precisely control the timing of texture updates (e.g. for staged rendering), by setting ImDrawData::Textures = NULL to handle this manually.
CIMGUI_IMPL_API void cImGui_ImplRSX_UpdateTexture(ImTextureData* tex);

#endif

#ifdef __cplusplus
}
#endif
