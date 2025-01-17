#include "main.h"

VkInstance g_instance;

PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;

VkResult create_instance();
void load_global_pointers();
void load_instance_pointers();

int main() {
    vkGetInstanceProcAddr = PLATFORM_load_vulkan_lib();
    if (vkGetInstanceProcAddr == VK_NULL_HANDLE) {
        printf("Unable to load Vulkan library\n");
        return -1;
    }

    CpdWindowInfo windowInfo = {
        .title = "Cpeed",
        .width = 800,
        .height = 600
    };
    CpdWindow window = PLATFORM_create_window(&windowInfo);

    load_global_pointers();

    VkResult result;

    result = create_instance();
    if (result != VK_SUCCESS) {
        printf_s("Unable to create Vulkan instance. Result code: %s\n", string_VkResult(result));
        return -1;
    }

    load_instance_pointers();

    // Do stuff
    printf("Vulkan instance handle: %p\n", g_instance);

    while (!PLATFORM_window_poll(window));
    // No mor stuff

    vkDestroyInstance(g_instance, VK_NULL_HANDLE);
    PLATFORM_window_destroy(window);
    PLATFORM_free_vulkan_lib();
}

void load_global_pointers() {
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
}

void load_instance_pointers() {
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(g_instance, "vkDestroyInstance");
}

VkResult create_instance() {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = 0,
        .pApplicationName = "Cpeed",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = 0,
        .engineVersion = 0,
        .applicationVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = 0,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = 0
    };

    return vkCreateInstance(&create_info, VK_NULL_HANDLE, &g_instance);
}
