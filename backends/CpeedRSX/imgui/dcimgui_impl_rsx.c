#include <malloc.h>

#include "dcimgui_impl_rsx.h"

typedef struct ImGui_ImplRSX_Data {
    gcmContextData* context;
} ImGui_ImplRSX_Data;

static ImGui_ImplRSX_Data* cImGui_ImplRSX_GetBackendData()
{
    return ImGui_GetCurrentContext() ? (ImGui_ImplRSX_Data*)ImGui_GetIO()->BackendRendererUserData : 0;
}

bool cImGui_ImplRSX_Init(gcmContextData* ctx) {
    ImGuiIO* io = ImGui_GetIO();
    
    CIMGUI_CHECKVERSION();
    IM_ASSERT(io->BackendRendererUserData == 0 && "Already initialized a renderer backend!");

    ImGui_ImplRSX_Data* bd = malloc(sizeof(ImGui_ImplRSX_Data));
    if (bd == 0) {
        return false;
    }

    io->BackendRendererUserData = (void*)bd;
    io->BackendRendererName = "imgui_impl_rsx";
    io->BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io->BackendFlags |= ImGuiBackendFlags_RendererHasTextures;  // We can honor ImGuiPlatformIO::Textures[] requests during render.

    ImGuiPlatformIO* platform_io = ImGui_GetPlatformIO();
    platform_io->Renderer_TextureMaxWidth = 4096;
    platform_io->Renderer_TextureMaxHeight = 4096;

    bd->context = ctx;

    return true;
}

void cImGui_ImplRSX_Shutdown(void) {
    ImGui_ImplRSX_Data* bd = cImGui_ImplRSX_GetBackendData();
    IM_ASSERT(bd != 0 && "No renderer backend to shutdown, or already shutdown?");
    
    ImGuiIO* io = ImGui_GetIO();
    ImGuiPlatformIO* platform_io = ImGui_GetPlatformIO();

    cImGui_ImplRSX_InvalidateDeviceObjects();

    io->BackendRendererName = 0;
    io->BackendRendererUserData = 0;
    io->BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    //platform_io.ClearRendererHandlers();

    free(bd);
}
void cImGui_ImplRSX_NewFrame(void) {
    //
}
void cImGui_ImplRSX_RenderDrawData(ImDrawData* draw_data) {
    //
}

bool cImGui_ImplRSX_CreateDeviceObjects(void) {
    return true;
}
void cImGui_ImplRSX_InvalidateDeviceObjects(void) {
    //
}

void cImGui_ImplRSX_UpdateTexture(ImTextureData* tex) {
    //
}
