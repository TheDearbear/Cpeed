#include "linuxmain.h"

void* g_vulkan;

CpdCompilePlatform PLATFORM_compile_platform() {
    return CpdCompilePlatform_Linux;
}

bool PLATFORM_initialize() {
    return true;
}

void PLATFORM_shutdown() { }

uint64_t PLATFORM_get_clock_usec() {
    struct timespec clock;
    if (clock_gettime(CLOCK_MONOTONIC, &clock) != 0) {
        return 0;
    }

    return ((uint64_t)clock.tv_sec * 1000000) + (clock.tv_nsec / 1000);
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

    const int extension_count = 1;

    const char** extensionNames = (const char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;

    return extensions;
}

void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
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
