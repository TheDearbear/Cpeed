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

    extensionNames[0] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
    extensionNames[1] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

    return extensions;
}

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_render_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 7;

    char** extensionNames = (char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    extensionNames[1] = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
    extensionNames[2] = VK_KHR_MULTIVIEW_EXTENSION_NAME;
    extensionNames[3] = VK_KHR_MAINTENANCE_2_EXTENSION_NAME;
    extensionNames[4] = VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME;
    extensionNames[5] = VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME;
    extensionNames[6] = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;

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

VkResult PLATFORM_create_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* surface) {
    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    if (vkCreateWin32SurfaceKHR == 0) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext = 0,
        .flags = 0,
        .hinstance = GetModuleHandleW(NULL),
        .hwnd = (HWND)window
    };

    return vkCreateWin32SurfaceKHR(instance, &create_info, VK_NULL_HANDLE, surface);
}

void PLATFORM_free_vulkan_extensions(CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free(extensions);
}
