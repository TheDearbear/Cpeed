#define VK_USE_PLATFORM_WAYLAND_KHR

#include <dlfcn.h>
#include <malloc.h>

#include "../../common/backend/vulkan/vulkan.h"
#include "../../platform/backend/vulkan.h"
#include "../linuxwayland.h"

void* g_vulkan_lib;

PFN_vkGetInstanceProcAddr load_vulkan_lib() {
    if (g_vulkan_lib == (void*)0) {
        g_vulkan_lib = dlopen("libvulkan.so.1", RTLD_LAZY);
    }

    if (g_vulkan_lib == (void*)0) {
        return VK_NULL_HANDLE;
    }

    PFN_vkGetInstanceProcAddr get_proc_addr = (PFN_vkGetInstanceProcAddr)dlsym(g_vulkan_lib, "vkGetInstanceProcAddr");
    if (get_proc_addr == (void*)0) {
        return VK_NULL_HANDLE;
    }

    return get_proc_addr;
}

void free_vulkan_lib() {
    if (g_vulkan_lib != (void*)0) {
        dlclose(g_vulkan_lib);
        g_vulkan_lib = (void*)0;
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

    extensionNames[0] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;

    return extensions;
}

void free_vulkan_extensions(const CpdVulkanExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
}

VkResult create_vulkan_surface(VkInstance instance, CpdWindow window, VkSurfaceKHR* surface) {
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
