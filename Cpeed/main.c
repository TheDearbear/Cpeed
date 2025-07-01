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

static void begin_rendering(CpdRenderer* renderer, VkCommandBuffer buffer);

static void load_global_pointers();
static void load_instance_pointers(CpdInstanceVulkanExtensions* extensions);

int main() {
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

    VkResult result;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    VkFence render_fence = VK_NULL_HANDLE;

    CpdInstanceVulkanExtensions instance_extensions;
    initialize_vulkan_instance_extensions(&instance_extensions);

    CpdRendererInitParams renderer_init_params = {
        .instance_extensions = &instance_extensions
    };

    result = create_instance(renderer_init_params.instance_extensions, &renderer_init_params.api_version, &renderer_init_params.max_api_version);
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

    result = create_fence(&renderer->render_device, &render_fence);
    if (result != VK_SUCCESS) {
        printf("Unable to create render fence. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = create_swapchain(renderer);
    if (result != VK_SUCCESS) {
        printf("Unable to create swapchain. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    result = create_primary_command_buffer(&renderer->render_device, renderer->render_device.graphics_family.pool, &buffer);
    if (result != VK_SUCCESS) {
        printf("Unable to create command buffer. Result code: %s\n", string_VkResult(result));
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

        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = VK_NULL_HANDLE
        };
        result = renderer->render_device.vkBeginCommandBuffer(buffer, &begin_info);
        if (result != VK_SUCCESS) {
            printf("Unable to begin command buffer. Result code: %s\n", string_VkResult(result));
            break;
        }

        if (wait_for_fence) {
            result = renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence, VK_FALSE, UINT64_MAX);
            if (result != VK_SUCCESS) {
                printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
                break;
            }
        }

        SWAPCHAIN_set_layout(&renderer->swapchain, &renderer->render_device, buffer,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        begin_rendering(renderer, buffer);

        renderer->render_device.vkCmdEndRendering(buffer);

        SWAPCHAIN_set_layout(&renderer->swapchain, &renderer->render_device, buffer,
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        result = renderer->render_device.vkEndCommandBuffer(buffer);
        if (result != VK_SUCCESS) {
            printf("Unable to end command buffer. Result code: %s\n", string_VkResult(result));
            break;
        }

        VkCommandBufferSubmitInfo buffer_submit_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = 0,
            .commandBuffer = buffer,
            .deviceMask = 0
        };

        VkSemaphore current_semaphore = renderer->swapchain.images[renderer->swapchain.current_image].semaphore;

        VkSemaphoreSubmitInfoKHR signal_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = 0,
            .semaphore = current_semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_NONE,
            .deviceIndex = 0
        };

        VkSubmitInfo2 submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = 0,
            .flags = 0,
            .waitSemaphoreInfoCount = 0,
            .pWaitSemaphoreInfos = 0,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &buffer_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_info
        };

        result = renderer->render_device.vkResetFences(renderer->render_device.handle, 1, &render_fence);
        if (result != VK_SUCCESS) {
            printf("Unable to reset render fence. Result code: %s\n", string_VkResult(result));
        }

        result = renderer->render_device.vkQueueSubmit2(renderer->render_device.graphics_family.queue, 1, &submit_info, render_fence);
        if (result != VK_SUCCESS) {
            printf("Unable to queue work. Result code: %s\n", string_VkResult(result));
        }

        renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &render_fence, VK_TRUE, UINT64_MAX);

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
    }

    printf("Goodbye!\n");

shutdown:
    if (renderer != 0) {
        if (renderer->render_device.handle != VK_NULL_HANDLE) {
            DEVICE_wait_idle(&renderer->render_device, false);
            renderer->render_device.vkFreeCommandBuffers(renderer->render_device.handle, renderer->render_device.graphics_family.pool, 1, &buffer);
            renderer->render_device.vkDestroyFence(renderer->render_device.handle, render_fence, VK_NULL_HANDLE);

            SWAPCHAIN_destroy(&renderer->swapchain, &renderer->render_device);
        }

        destroy_renderer(renderer);
    }
    vkDestroyInstance(g_instance, VK_NULL_HANDLE);

    PLATFORM_window_destroy(g_window);
    PLATFORM_free_vulkan_lib();
}

static void begin_rendering(CpdRenderer* renderer, VkCommandBuffer buffer) {
    CpdImage* image = &renderer->swapchain.images[renderer->swapchain.current_image].image;

    VkRenderingAttachmentInfo attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image->view,
        .imageLayout = image->layout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color.float32[0] = 1.0f,
        .clearValue.color.float32[1] = 0,
        .clearValue.color.float32[2] = 0,
        .clearValue.color.float32[3] = 1.0f
    };
    
    VkRenderingInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea.offset = (VkOffset2D){ 0, 0 },
        .renderArea.extent.width = renderer->swapchain.size.width,
        .renderArea.extent.height = renderer->swapchain.size.height,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment
    };

    renderer->render_device.vkCmdBeginRendering(buffer, &info);
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
