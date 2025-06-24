#include "linuxmain.h"

void* g_vulkan;

CpdCompilePlatform PLATFORM_compile_platform() {
    return CpdCompilePlatform_Linux;
}

PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib() {
    if (g_vulkan == (void*)0) {
        g_vulkan = dlopen("libvulkan.so.1", RTLD_LAZY);
    }

    if (g_vulkan == (void*)0) {
        return VK_NULL_HANDLE;
    }

    PFN_vkGetInstanceProcAddr get_proc_addr = (PFN_vkGetInstanceProcAddr)dlsym(g_vulkan, "vkGetInstanceProcAddr");
    if (get_proc_addr == (void*)0) {
        return VK_NULL_HANDLE;
    }

    return get_proc_addr;
}

void PLATFORM_free_vulkan_lib() {
    if (g_vulkan != (void*)0) {
        dlclose(g_vulkan);
        g_vulkan = (void*)0;
    }
}

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_instance_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 2;

    const char** extensionNames = (const char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    extensionNames[1] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

    return extensions;
}

const CpdPlatformExtensions* PLATFORM_alloc_vulkan_render_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 7;

    const char** extensionNames = (const char**)malloc(extension_count * sizeof(char*));
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
    PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
    if (vkCreateWaylandSurfaceKHR == 0) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    CpdWaylandWindow* wl_window = (CpdWaylandWindow*)window;

    VkWaylandSurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .pNext = 0,
        .flags = 0,
        .display = g_display,
        .surface = wl_window->surface
    };

    return vkCreateWaylandSurfaceKHR(instance, &create_info, VK_NULL_HANDLE, surface);
}

void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
}
