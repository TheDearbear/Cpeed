#include "linuxmain.h"

void* g_vulkan;

CompilePlatform PLATFORM_compile_platform() {
    return CompilePlatform_Linux;
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
