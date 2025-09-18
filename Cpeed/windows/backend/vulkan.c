#define VK_USE_PLATFORM_WIN32_KHR

#include "../../vulkan/vulkan.h"
#include "../../platform/backend/vulkan.h"
#include "../winmain.h"

HMODULE g_vulkan_lib;

PFN_vkGetInstanceProcAddr load_vulkan_lib() {
    if (g_vulkan_lib == NULL) {
        g_vulkan_lib = LoadLibraryW(L"vulkan-1");
    }

    if (g_vulkan_lib == NULL) {
        return VK_NULL_HANDLE;
    }

    PFN_vkGetInstanceProcAddr get_proc_addr = (PFN_vkGetInstanceProcAddr)GetProcAddress(g_vulkan_lib, "vkGetInstanceProcAddr");
    if (get_proc_addr == NULL) {
        return VK_NULL_HANDLE;
    }

    return get_proc_addr;
}

void free_vulkan_lib() {
    if (g_vulkan_lib != NULL) {
        FreeLibrary(g_vulkan_lib);
        g_vulkan_lib = NULL;
    }
}

const CpdVulkanExtensions* alloc_vulkan_instance_extensions() {
    CpdVulkanExtensions* extensions = (CpdVulkanExtensions*)malloc(sizeof(CpdVulkanExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 1;

    const char** extensionNames = (const char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;

    return extensions;
}

void free_vulkan_extensions(const CpdVulkanExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
}

VkResult create_vulkan_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* surface) {
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
