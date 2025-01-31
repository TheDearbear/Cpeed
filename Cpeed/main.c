#include "main.h"

#define GET_INSTANCE_PROC_ADDR(instance, name) name = (PFN_ ## name)vkGetInstanceProcAddr(instance, #name)

VkInstance g_instance;

VkResult create_instance();
VkResult create_command_buffer(CpdDevice* device, VkCommandPool pool, VkCommandBuffer* buffer);
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

    VkCommandBuffer buffer = VK_NULL_HANDLE;
    result = create_command_buffer(&renderer->render_device, renderer->render_device.graphics_family.pool, &buffer);
    if (result != VK_SUCCESS) {
        printf("Unable to create command buffer. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = RENDERER_select_ui_device(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to select ui device. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    VkSurfaceKHR surface;
    result = PLATFORM_create_surface(g_instance, window, &surface);
    if (result != VK_SUCCESS) {
        printf("Unable to create surface. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    {
        CpdWindowSize size = PLATFORM_get_window_size(window);
        result = RENDERER_set_surface(renderer, surface, &size);
        if (result != VK_SUCCESS) {
            printf("Unable to initialize surface for rendering. Result code: %s\n", string_VkResult(result));
            goto shutdown;
        }
    }

    while (!PLATFORM_window_poll(window)) {
        bool resized = PLATFORM_window_resized(window);
        if (resized) {
            CpdWindowSize size = PLATFORM_get_window_size(window);
            
            result = RENDERER_update_surface_size(renderer, &size);
            if (result != VK_SUCCESS) {
                printf("Unable to update surface size. Result code: %s\n", string_VkResult(result));
                continue;
            }
        }

        result = RENDERER_reset_pools(renderer);
        if (result != VK_SUCCESS) {
            printf("Unable to reset command pools. Result code: %s\n", string_VkResult(result));
            break;
        }

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .pInheritanceInfo = VK_NULL_HANDLE
        };
        result = renderer->render_device.vkBeginCommandBuffer(buffer, &begin_info);
        if (result != VK_SUCCESS) {
            printf("Unable to begin command buffer. Result code: %s\n", string_VkResult(result));
            break;
        }

        result = renderer->render_device.vkEndCommandBuffer(buffer);
        if (result != VK_SUCCESS) {
            printf("Unable to end command buffer. Result code: %s\n", string_VkResult(result));
            break;
        }

        VkCommandBufferSubmitInfo buffer_submit_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = VK_NULL_HANDLE,
            .commandBuffer = buffer,
            .deviceMask = 0
        };

        VkSubmitInfo2KHR submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .waitSemaphoreInfoCount = 0,
            .pWaitSemaphoreInfos = VK_NULL_HANDLE,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &buffer_submit_info,
            .signalSemaphoreInfoCount = 0,
            .pSignalSemaphoreInfos = VK_NULL_HANDLE
        };

        result = renderer->render_device.vkQueueSubmit2KHR(renderer->render_device.graphics_family.queue, 1, &submit_info, VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            printf("Unable to queue work. Result code: %s\n", string_VkResult(result));
        }

        result = RENDERER_wait_idle(renderer);
        if (result != VK_SUCCESS) {
            printf("Unable to wait for idle. Result code: %s\n", string_VkResult(result));
        }
    }

    printf("Goodbye!\n");

shutdown:
    renderer->render_device.vkFreeCommandBuffers(renderer->render_device.handle, renderer->render_device.graphics_family.pool, 1, &buffer);

    RENDERER_destroy(renderer);

#pragma warning(push)
#pragma warning(disable:6001)
    vkDestroySurfaceKHR(g_instance, surface, VK_NULL_HANDLE);
#pragma warning(pop)

    vkDestroyInstance(g_instance, VK_NULL_HANDLE);
    PLATFORM_window_destroy(window);
    PLATFORM_free_vulkan_lib();
}

void load_global_pointers() {
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkCreateInstance);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceExtensionProperties);
}

void load_instance_pointers() {
    GET_INSTANCE_PROC_ADDR(g_instance, vkDestroyInstance);
    GET_INSTANCE_PROC_ADDR(g_instance, vkEnumeratePhysicalDevices);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetPhysicalDeviceProperties);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetPhysicalDeviceQueueFamilyProperties);
    GET_INSTANCE_PROC_ADDR(g_instance, vkCreateDevice);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetDeviceProcAddr);
    GET_INSTANCE_PROC_ADDR(g_instance, vkDestroySurfaceKHR);
    GET_INSTANCE_PROC_ADDR(g_instance, vkEnumerateDeviceExtensionProperties);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(g_instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
}

static VkResult validate_extensions(char** extensions, unsigned int extension_count) {
    uint32_t property_count;
    VkResult result = vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &property_count, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkExtensionProperties* properties = (VkExtensionProperties*)malloc(property_count * sizeof(VkExtensionProperties));
    if (properties == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &property_count, properties);
    if (result != VK_SUCCESS) {
        free(properties);
        return result;
    }

    bool missing = false;
    for (unsigned int i = 0; i < extension_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < property_count; j++) {
            #pragma warning(push)
            #pragma warning(disable:6385)
            if (strcmp(extensions[i], properties[j].extensionName) == 0) {
            #pragma warning(pop)
                found = true;
                break;
            }
        }

        if (!found) {
            missing = true;
            printf("Instance extension missing: %s\n", extensions[i]);
        }
    }

    free(properties);
    return missing ? VK_ERROR_EXTENSION_NOT_PRESENT : VK_SUCCESS;
}

VkResult create_command_buffer(CpdDevice* cpeed_device, VkCommandPool pool, VkCommandBuffer* buffer) {
    VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    return cpeed_device->vkAllocateCommandBuffers(cpeed_device->handle, &allocate_info, buffer);
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

    const int base_extension_count = 1;

    unsigned int all_extensions_count = extensions->count + base_extension_count;
    char** all_extensions = (char**)malloc((size_t)all_extensions_count * sizeof(char*));
    if (all_extensions == 0) {
        PLATFORM_free_vulkan_extensions(extensions);
        printf("Unable to get all required Vulkan instance extensions\n");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    all_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
    for (unsigned int i = 0; i < extensions->count; i++) {
        all_extensions[i + base_extension_count] = extensions->extensions[i];
    }

    VkResult result = validate_extensions(all_extensions, all_extensions_count);
    if (result != VK_SUCCESS) {
        return result;
    }

    for (unsigned int i = 0; i < all_extensions_count; i++) {
        printf("Enabling instance extension: %s\n", all_extensions[i]);
    }

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = 0,
        .enabledExtensionCount = all_extensions_count,
        .ppEnabledExtensionNames = all_extensions_count > 0 ? all_extensions : 0
    };

    result = vkCreateInstance(&create_info, VK_NULL_HANDLE, &g_instance);

    PLATFORM_free_vulkan_extensions(extensions);

    return result;
}
