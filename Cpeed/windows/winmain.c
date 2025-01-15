#include "winmain.h"

HMODULE g_vulkan;

PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib() {
    if (g_vulkan == NULL) {
        g_vulkan = LoadLibraryW(L"vulkan-1");
    }

    if (g_vulkan == NULL) { 
        return VK_NULL_HANDLE;
    }

    PFN_vkGetInstanceProcAddr get_proc_addr = (PFN_vkGetInstanceProcAddr)GetProcAddress(g_vulkan, "vkGetInstanceProcAddr");
    if (get_proc_addr == NULL) {
        return VK_NULL_HANDLE;
    }

    return get_proc_addr;
}

void PLATFORM_free_vulkan_lib() {
    if (g_vulkan != NULL) {
        FreeLibrary(g_vulkan);
        g_vulkan = NULL;
    }
}
