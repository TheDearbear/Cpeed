#include "main.h"
#include "common/renderer.h"

VkInstance g_instance;

PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

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
        printf("Unable to create Vulkan instance. Result code: %s\n", string_VkResult(result));
        return -1;
    }

    load_instance_pointers();

    CpdRenderer* renderer = RENDERER_create(g_instance);
    result = RENDERER_select_render_device(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to select render device. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = RENDERER_select_ui_device(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to select ui device. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    while (!PLATFORM_window_poll(window)) {
        // Do stuff
    }

    printf("Goodbye!\n");

shutdown:
    RENDERER_destroy(renderer);
    vkDestroyInstance(g_instance, VK_NULL_HANDLE);
    PLATFORM_window_destroy(window);
    PLATFORM_free_vulkan_lib();
}

void load_global_pointers() {
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
}

void load_instance_pointers() {
    vkDestroyInstance = (PFN_vkDestroyInstance)vkGetInstanceProcAddr(g_instance, "vkDestroyInstance");
    vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(g_instance, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceProperties");
    vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    vkCreateDevice = (PFN_vkCreateDevice)vkGetInstanceProcAddr(g_instance, "vkCreateDevice");
    vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(g_instance, "vkGetDeviceProcAddr");
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

    CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_instance_extensions();
    if (extensions == 0) {
        printf("Unable to get required Vulkan instance extensions\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    for (unsigned int i = 0; i < extensions->count; i++) {
        printf("Enabling instance extension: %s\n", extensions->extensions[i]);
    }

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = 0,
        .enabledExtensionCount = extensions->count,
        .ppEnabledExtensionNames = extensions->count > 0 ? extensions->extensions : 0
    };

    VkResult result = vkCreateInstance(&create_info, VK_NULL_HANDLE, &g_instance);

    PLATFORM_free_vulkan_extensions(extensions);

    return result;
}
