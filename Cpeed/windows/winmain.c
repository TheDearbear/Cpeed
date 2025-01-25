#include "winmain.h"

HMODULE g_vulkan;

CpdCompilePlatform PLATFORM_compile_platform() {
    return CpdCompilePlatform_Windows;
}

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

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_instance_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 2;

    char** extensionNames = (char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_SURFACE_EXTENSION_NAME;
    extensionNames[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

    return extensions;
}

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_render_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    extensions->count = 0;

    return extensions;
}

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_ui_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    extensions->count = 0;

    return extensions;
}

void PLATFORM_free_vulkan_extensions(CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free(extensions);
}
