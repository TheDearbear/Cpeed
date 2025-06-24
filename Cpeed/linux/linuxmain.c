#include "linuxmain.h"

void* g_vulkan;

CpdCompilePlatform PLATFORM_compile_platform() {
    return CpdCompilePlatform_Linux;
}

PFN_vkGetInstanceProcAddr PLATFORM_load_vulkan_lib() {
    if (g_vulkan == (void*)0) {
        g_vulkan = dlopen("libvulkan.so", RTLD_LAZY);
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

    extensions->count = 0;

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

void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
}
