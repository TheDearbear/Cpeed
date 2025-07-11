#include "common/rendering/rendering.h"
#include "main.h"

#define GET_INSTANCE_PROC_ADDR(instance, name) name = (PFN_ ## name)vkGetInstanceProcAddr(instance, #name)
#define GET_INSTANCE_PROC_ADDR_SUFFIX(instance, name, suffix) name = (PFN_ ## name)vkGetInstanceProcAddr(instance, #name suffix)

#define GET_INSTANCE_PROC_ADDR_EXTENSION(instance, name, suffix, extension, extensions) { \
    if ((extensions)->extension.load_method == CpdVulkanExtensionLoadMethod_FromCore) { GET_INSTANCE_PROC_ADDR(instance, name); } \
    else if ((extensions)->extension.load_method == CpdVulkanExtensionLoadMethod_FromExtension) { GET_INSTANCE_PROC_ADDR_SUFFIX(instance, name, suffix); } \
    else { name = VK_NULL_HANDLE; } \
}

VkInstance g_instance;
CpdWindow g_window;

static VkResult create_instance(CpdInstanceVulkanExtensions* instance_extensions, uint32_t* used_api_version, uint32_t* max_supported_api_version);
static VkResult create_renderer(CpdRenderer** renderer, CpdRendererInitParams* params);
static VkResult create_swapchain(CpdRenderer* renderer);

static void destroy_renderer(CpdRenderer* renderer);

static void load_global_pointers();
static void load_instance_pointers(CpdInstanceVulkanExtensions* extensions);

int main() {
    if (!PLATFORM_initialize()) {
        printf("Unable to initialize platform-specific information\n");
        return -1;
    }

    vkGetInstanceProcAddr = PLATFORM_load_vulkan_lib();
    if (vkGetInstanceProcAddr == VK_NULL_HANDLE) {
        printf("Unable to load Vulkan library\n");
        return -1;
    }

    CpdWindowInfo windowInfo = {
        .title = "Cpeed",
        .size.width = 800,
        .size.height = 600
    };
    g_window = PLATFORM_create_window(&windowInfo);

    load_global_pointers();

    CpdInstanceVulkanExtensions instance_extensions;
    initialize_vulkan_instance_extensions(&instance_extensions);

    CpdRendererInitParams renderer_init_params = {
        .instance_extensions = &instance_extensions
    };

    VkResult result = create_instance(renderer_init_params.instance_extensions, &renderer_init_params.api_version, &renderer_init_params.max_api_version);
    if (result != VK_SUCCESS) {
        printf("Unable to create Vulkan instance. Result code: %s\n", string_VkResult(result));
        return -1;
    }

    renderer_init_params.instance = g_instance;

    CpdRenderer* renderer = 0;
    result = create_renderer(&renderer, &renderer_init_params);
    if (result != VK_SUCCESS) {
        printf("Unable to create renderer. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = RENDERING_initialize(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to initialize rendering module. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = create_swapchain(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to create swapchain. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    while (!PLATFORM_window_poll(g_window)) {
        bool resized = PLATFORM_window_resized(g_window);
        if (resized) {
            CpdSize size = PLATFORM_get_window_size(g_window);

            DEVICE_wait_idle(&renderer->render_device, false);
            
            result = SWAPCHAIN_resize(&renderer->swapchain, &renderer->render_device, &renderer->surface, &size);
            if (result != VK_SUCCESS) {
                printf("Unable to update surface size. Result code: %s\n", string_VkResult(result));
                continue;
            }

            result = RENDERING_resize(renderer, size);
            if (result != VK_SUCCESS) {
                printf("Unable to adapt for new surface size. Result code: %s\n", string_VkResult(result));
                continue;
            }
        }

        result = RENDERER_reset_pools(renderer);
        if (result != VK_SUCCESS) {
            printf("Unable to reset command pools. Result code: %s\n", string_VkResult(result));
            break;
        }

        result = RENDERER_acquire_next_image(renderer, false);
        bool wait_for_fence = result == VK_NOT_READY;

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_NOT_READY) {
            printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
            break;
        }

        result = RENDERING_frame(renderer, wait_for_fence);
        if (result != VK_SUCCESS) {
            printf("Unable to draw new frame. Result code: %s\n", string_VkResult(result));
            break;
        }

        VkSemaphore current_semaphore = renderer->swapchain.images[renderer->swapchain.current_image].semaphore;

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = 0,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &current_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &renderer->swapchain.handle,
            .pImageIndices = &renderer->swapchain.current_image,
            .pResults = VK_NULL_HANDLE
        };
        result = renderer->render_device.vkQueuePresentKHR(renderer->render_device.graphics_family.queue, &present_info);
        if (result != VK_SUCCESS) {
            printf("Unable to present swapchain. Result code: %s\n", string_VkResult(result));
        }

        renderer->last_frame_end = PLATFORM_get_clock_usec();
    }

    printf("Goodbye!\n");

shutdown:
    if (renderer != 0) {
        if (renderer->render_device.handle != VK_NULL_HANDLE) {
            DEVICE_wait_idle(&renderer->render_device, false);

            RENDERING_shutdown(renderer);
            SWAPCHAIN_destroy(&renderer->swapchain, &renderer->render_device);
        }

        destroy_renderer(renderer);
    }
    vkDestroyInstance(g_instance, VK_NULL_HANDLE);

    PLATFORM_window_destroy(g_window);
    PLATFORM_free_vulkan_lib();
    PLATFORM_shutdown();
}

static VkResult create_renderer(CpdRenderer** renderer, CpdRendererInitParams* params) {
    *renderer = RENDERER_create(params);

    VkResult result = RENDERER_select_render_device(*renderer);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = RENDERER_select_ui_device(*renderer);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSurfaceKHR surface;
    result = PLATFORM_create_surface(params->instance, g_window, &surface);
    if (result != VK_SUCCESS) {
        return result;
    }

    return SURFACE_initialize(&(*renderer)->surface, &(*renderer)->render_device, surface);
}

static VkResult create_swapchain(CpdRenderer* renderer) {
    CpdSize size = PLATFORM_get_window_size(g_window);

    VkResult result = SWAPCHAIN_create(&renderer->swapchain, &renderer->render_device, &renderer->surface, &size);
    if (result != VK_SUCCESS) {
        return result;
    }

    return create_fence(&renderer->render_device, &renderer->swapchain_image_fence);
}

static void destroy_renderer(CpdRenderer* renderer) {
    VkSurfaceKHR surface = renderer->surface.handle;
    VkInstance instance = renderer->instance;

    if (renderer->render_device.handle != VK_NULL_HANDLE) {
        renderer->render_device.vkDestroyFence(renderer->render_device.handle, renderer->swapchain_image_fence, VK_NULL_HANDLE);
    }

    RENDERER_destroy(renderer);
    vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
}

static void load_global_pointers() {
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceVersion);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkCreateInstance);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceExtensionProperties);
}

static void load_instance_pointers(CpdInstanceVulkanExtensions* extensions) {
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

    GET_INSTANCE_PROC_ADDR_EXTENSION(g_instance, vkGetPhysicalDeviceProperties2, "KHR", get_physical_device_properties2, extensions);
    GET_INSTANCE_PROC_ADDR_EXTENSION(g_instance, vkGetPhysicalDeviceQueueFamilyProperties2, "KHR", get_physical_device_properties2, extensions);
}

static VkResult validate_extensions(
    const char** extensions, uint32_t extension_count,
    VkExtensionProperties* properties, uint32_t property_count
) {
    bool missing = false;
    for (unsigned int i = 0; i < extension_count; i++) {
        bool found = false;
        for (uint32_t j = 0; j < property_count; j++) {
            if (strcmp(extensions[i], properties[j].extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            missing = true;
            printf("Instance extension missing: %s\n", extensions[i]);
        }
    }

    return missing ? VK_ERROR_EXTENSION_NOT_PRESENT : VK_SUCCESS;
}

static VkResult create_instance(CpdInstanceVulkanExtensions* instance_extensions, uint32_t* used_api_version, uint32_t* max_supported_api_version) {
    VkResult result;

    uint32_t api_version = VK_API_VERSION_1_0;
    uint32_t max_api_version = VK_API_VERSION_1_0;
    if (vkEnumerateInstanceVersion != 0) {
        result = vkEnumerateInstanceVersion(&max_api_version);
        
        api_version = result == VK_SUCCESS ? min_u32(max_api_version, VK_API_VERSION_1_3) : VK_API_VERSION_1_1;
    }

    // == Instance Extensions

    uint32_t property_count;

    result = vkEnumerateInstanceExtensionProperties(0, &property_count, 0);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkExtensionProperties* properties = (VkExtensionProperties*)malloc(property_count * sizeof(VkExtensionProperties));
    if (properties == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkEnumerateInstanceExtensionProperties(0, &property_count, properties);
    if (result != VK_SUCCESS) {
        return result;
    }

    CpdVulkanExtension* extensions = (CpdVulkanExtension*)instance_extensions;
    uint32_t extension_count = GET_EXTENSIONS_COUNT(CpdInstanceVulkanExtensions);
    uint32_t extensions_found = 0;
    uint32_t loaded_from_extension = 0;

    for (uint32_t i = 0; i < property_count && extensions_found < extension_count; i++) {
        for (uint32_t j = 0; j < extension_count; j++) {
            CpdVulkanExtension* extension = &extensions[j];

            if (strcmp(properties[i].extensionName, extension->name) != 0) {
                continue;
            }

            extensions_found++;

            if (extension->promoted_version != UINT32_MAX) {
                if (api_version >= extension->promoted_version) {
                    extension->load_method = CpdVulkanExtensionLoadMethod_FromCore;
                    break;
                }

                if (max_api_version >= extension->promoted_version) {
                    api_version = extension->promoted_version;
                    extension->load_method = CpdVulkanExtensionLoadMethod_FromCore;
                    break;
                }
            }

            extension->load_method = CpdVulkanExtensionLoadMethod_FromExtension;
            loaded_from_extension++;
            break;
        }
    }

    const CpdPlatformExtensions* platform_extensions = PLATFORM_alloc_vulkan_instance_extensions();
    uint32_t platform_extensions_count = platform_extensions->count;

    const uint32_t base_extension_count = 1;

    const char** all_extensions = (const char**)malloc((size_t)(base_extension_count + platform_extensions_count + loaded_from_extension) * sizeof(const char*));
    if (all_extensions == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    all_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;

    for (uint32_t i = 0; i < platform_extensions_count; i++) {
        all_extensions[base_extension_count + i] = platform_extensions->extensions[i];
    }

    PLATFORM_free_vulkan_extensions(platform_extensions);

    uint32_t copied_extensions = 0;
    for (uint32_t i = 0; i < extension_count && copied_extensions < loaded_from_extension; i++) {
        if (extensions[i].load_method != CpdVulkanExtensionLoadMethod_FromExtension) {
            continue;
        }

        all_extensions[base_extension_count + platform_extensions_count + copied_extensions++] = extensions[i].name;
    }

    uint32_t all_extensions_count = base_extension_count + platform_extensions_count + copied_extensions;

    result = validate_extensions(all_extensions, all_extensions_count, properties, property_count);
    free(properties);

    if (result != VK_SUCCESS) {
        free(all_extensions);
        return result;
    }

    for (unsigned int i = 0; i < all_extensions_count; i++) {
        printf("Enabling instance extension: %s\n", all_extensions[i]);
    }

    if (api_version != 0) {
        *used_api_version = api_version;
    }
    if (max_api_version != 0) {
        *max_supported_api_version = max_api_version;
    }

    // == Instance Creating

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = 0,
        .pApplicationName = "Cpeed",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = 0,
        .engineVersion = 0,
        .apiVersion = api_version
    };

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

    free(all_extensions);

    if (result == VK_SUCCESS) {
        load_instance_pointers(instance_extensions);
    }

    return result;
}
