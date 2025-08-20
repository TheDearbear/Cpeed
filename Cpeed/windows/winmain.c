#include "winmain.h"

HMODULE g_vulkan;

LARGE_INTEGER g_counter_frequency;
ATOM g_window_class;

CpdCompilePlatform PLATFORM_compile_platform() {
    return CpdCompilePlatform_Windows;
}

uint64_t PLATFORM_get_clock_usec() {
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    return counter.QuadPart * 1000000 / g_counter_frequency.QuadPart;
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

void PLATFORM_free_vulkan_extensions(const CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
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

void cleanup_input_queue(CpdInputEvent* queue, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        CpdInputEvent* event = &queue[i];

        if (event->type == CpdInputEventType_TextInput) {
            free(event->data.text_input.text);
            event->data.text_input.text = 0;
        }
    }
}
