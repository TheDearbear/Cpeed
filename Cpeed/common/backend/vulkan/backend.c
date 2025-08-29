#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "../../../platform/backend/vulkan.h"
#include "rendering/rendering.h"
#include "backend.h"
#include "renderer.h"
#include "shortcuts.h"
#include "vulkan.h"

static void load_global_pointers() {
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceVersion);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkCreateInstance);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceExtensionProperties);
}

static bool initialize_backend() {
    vkGetInstanceProcAddr = load_vulkan_lib();

    if (vkGetInstanceProcAddr != 0) {
        load_global_pointers();
        return true;
    }

    return false;
}

static void shutdown_backend() {
    free_vulkan_lib();
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

static void shutdown_window(CpdBackendHandle backend) {
    CpdRenderer* renderer = (CpdRenderer*)backend;

    if (renderer == 0) {
        return;
    }

    VkInstance instance = renderer->instance;

    if (renderer->render_device.handle != VK_NULL_HANDLE) {
        DEVICE_wait_idle(&renderer->render_device, false);

        RENDERING_shutdown(renderer);
        SWAPCHAIN_destroy(&renderer->swapchain, &renderer->render_device);
    }

    destroy_renderer(renderer);

    vkDestroyInstance(instance, VK_NULL_HANDLE);
}

static VkResult create_renderer(CpdWindow window, CpdRenderer** renderer, CpdRendererInitParams* params) {
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
    result = create_vulkan_surface((*renderer)->instance, window, &surface);
    if (result != VK_SUCCESS) {
        return result;
    }

    return SURFACE_initialize(&(*renderer)->surface, &(*renderer)->render_device, surface);
}

static VkResult create_swapchain(CpdWindow window, CpdRenderer* renderer) {
    CpdSize size = window_size(window);

    VkResult result = SWAPCHAIN_create(&renderer->swapchain, &renderer->render_device, &renderer->surface, &size);
    if (result != VK_SUCCESS) {
        return result;
    }

    return create_fence(&renderer->render_device, &renderer->swapchain_image_fence);
}

static void load_instance_pointers(VkInstance instance, CpdInstanceVulkanExtensions* extensions) {
    GET_INSTANCE_PROC_ADDR(instance, vkDestroyInstance);
    GET_INSTANCE_PROC_ADDR(instance, vkEnumeratePhysicalDevices);
    GET_INSTANCE_PROC_ADDR(instance, vkGetPhysicalDeviceProperties);
    GET_INSTANCE_PROC_ADDR(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    GET_INSTANCE_PROC_ADDR(instance, vkCreateDevice);
    GET_INSTANCE_PROC_ADDR(instance, vkGetDeviceProcAddr);
    GET_INSTANCE_PROC_ADDR(instance, vkDestroySurfaceKHR);
    GET_INSTANCE_PROC_ADDR(instance, vkEnumerateDeviceExtensionProperties);
    GET_INSTANCE_PROC_ADDR(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    GET_INSTANCE_PROC_ADDR_EXTENSION(instance, vkGetPhysicalDeviceProperties2, "KHR", extensions->get_physical_device_properties2);
    GET_INSTANCE_PROC_ADDR_EXTENSION(instance, vkGetPhysicalDeviceQueueFamilyProperties2, "KHR", extensions->get_physical_device_properties2);
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

static VkResult create_instance(CpdRendererInitParams* init_params) {
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

    CpdVulkanExtension* extensions = (CpdVulkanExtension*)init_params->instance_extensions;
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

    const CpdVulkanExtensions* platform_extensions = alloc_vulkan_instance_extensions();
    uint32_t platform_extensions_count = platform_extensions->count;

    const uint32_t base_extension_count = 1;

    const char** all_extensions = (const char**)malloc((size_t)(base_extension_count + platform_extensions_count + loaded_from_extension) * sizeof(const char*));
    if (all_extensions == 0) {
        free_vulkan_extensions(platform_extensions);
        free(properties);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    all_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;

    for (uint32_t i = 0; i < platform_extensions_count; i++) {
        all_extensions[base_extension_count + i] = platform_extensions->extensions[i];
    }

    free_vulkan_extensions(platform_extensions);

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

    init_params->api_version = api_version;
    init_params->max_api_version = max_api_version;

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

    result = vkCreateInstance(&create_info, VK_NULL_HANDLE, &init_params->instance);

    free(all_extensions);

    if (result == VK_SUCCESS) {
        load_instance_pointers(init_params->instance, init_params->instance_extensions);
    }

    return result;
}

static CpdBackendHandle initialize_window(CpdWindow window) {
    CpdInstanceVulkanExtensions instance_extensions;
    initialize_vulkan_instance_extensions(&instance_extensions);

    CpdRendererInitParams renderer_init_params = {
        .instance_extensions = &instance_extensions
    };

    VkResult result = create_instance(&renderer_init_params);
    if (result != VK_SUCCESS) {
        printf("Unable to create Vulkan instance. Result code: %s\n", string_VkResult(result));
        return 0;
    }

    CpdRenderer* renderer = 0;
    result = create_renderer(window, &renderer, &renderer_init_params);
    if (result != VK_SUCCESS) {
        printf("Unable to create renderer. Result code: %s\n", string_VkResult(result));
        vkDestroyInstance(renderer_init_params.instance, VK_NULL_HANDLE);
        shutdown_window((CpdBackendHandle)renderer);
        return 0;
    }

    result = RENDERING_initialize(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to initialize rendering module. Result code: %s\n", string_VkResult(result));
        shutdown_window((CpdBackendHandle)renderer);
        return 0;
    }

    result = create_swapchain(window, renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to create swapchain. Result code: %s\n", string_VkResult(result));
        shutdown_window((CpdBackendHandle)renderer);
        return 0;
    }

    return (CpdBackendHandle)renderer;
}

static CpdBackendVersion get_version(CpdBackendHandle cpeed_backend) {
    uint32_t raw_version = 0;

    if (vkEnumerateInstanceVersion == 0 || vkEnumerateInstanceVersion(&raw_version) != VK_SUCCESS) {
        return (CpdBackendVersion) {
            .major = 1,
            .minor = 0
        };
    }

    return (CpdBackendVersion) {
        .major = VK_API_VERSION_MAJOR(raw_version),
        .minor = VK_API_VERSION_MINOR(raw_version)
    };
}

static bool resize(CpdBackendHandle cpeed_backend, CpdSize new_size) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

    DEVICE_wait_idle(&renderer->render_device, false);

    VkResult result = SWAPCHAIN_resize(&renderer->swapchain, &renderer->render_device, &renderer->surface, &new_size);
    if (result != VK_SUCCESS) {
        printf("Unable to update surface size. Result code: %s\n", string_VkResult(result));
        return false;
    }

    result = RENDERING_resize(renderer, new_size);
    if (result != VK_SUCCESS) {
        printf("Unable to adapt for new surface size. Result code: %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

static void input(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window, const CpdInputEvent* input_events, uint32_t input_event_count) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

    RENDERING_input(renderer, cpeed_window, input_events, input_event_count);
}

static bool should_frame(CpdBackendHandle cpeed_backend, CpdWindow cpeed_window) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

    renderer->render_settings->allow_render = window_present_allowed(cpeed_window);

    return renderer->render_settings->allow_render && !renderer->render_settings->force_disable_render;
}

static bool pre_frame(CpdBackendHandle cpeed_backend) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

    VkResult result = RENDERER_reset_pools(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to reset command pools. Result code: %s\n", string_VkResult(result));
        return false;
    }

    result = RENDERER_acquire_next_image(renderer, false);
    renderer->swapchain.wait_for_acquire = result == VK_NOT_READY;

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_NOT_READY) {
        printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

static bool frame(CpdBackendHandle cpeed_backend) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

    VkResult result = RENDERING_frame(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to draw new frame. Result code: %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

static bool post_frame(CpdBackendHandle cpeed_backend) {
    CpdRenderer* renderer = (CpdRenderer*)cpeed_backend;

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
    VkResult result = renderer->render_device.vkQueuePresentKHR(renderer->render_device.graphics_family.queue, &present_info);
    if (result != VK_SUCCESS) {
        printf("Unable to present swapchain. Result code: %s\n", string_VkResult(result));
    }

    renderer->last_frame_end = get_clock_usec();
    return true;
}

void get_vulkan_backend_implementation(CpdBackendImplementation* implementation) {
    implementation->type = CpdPlatformBackendFlags_Vulkan;
    implementation->initialize_backend = initialize_backend;
    implementation->shutdown_backend = shutdown_backend;
    implementation->initialize_window = initialize_window;
    implementation->shutdown_window = shutdown_window;
    implementation->get_version = get_version;
    implementation->resize = resize;
    implementation->input = input;
    implementation->should_frame = should_frame;
    implementation->pre_frame = pre_frame;
    implementation->frame = frame;
    implementation->post_frame = post_frame;
}
