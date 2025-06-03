#include "main.h"

#define GET_INSTANCE_PROC_ADDR(instance, name) name = (PFN_ ## name)vkGetInstanceProcAddr(instance, #name)

VkInstance g_instance;
CpdWindow g_window;

static VkResult create_instance();
static VkResult create_renderer(CpdRenderer** renderer);
static VkResult create_semaphores(CpdRenderer* renderer, VkSemaphore* acquire, VkSemaphore* render);
static VkResult create_swapchain(CpdRenderer* renderer);

static void destroy_renderer(CpdRenderer* renderer);

static void begin_rendering(CpdRenderer* renderer, VkCommandBuffer buffer);

static void load_global_pointers();
static void load_instance_pointers();

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
    VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
    VkSemaphore render_semaphore = VK_NULL_HANDLE;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    VkFence render_fence = VK_NULL_HANDLE;

    result = create_instance();
    if (result != VK_SUCCESS) {
        printf("Unable to create Vulkan instance. Result code: %s\n", string_VkResult(result));
        return -1;
    }

    CpdRenderer* renderer;
    result = create_renderer(&renderer);
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

    result = create_semaphores(renderer, &acquire_semaphore, &render_semaphore);
    if (result != VK_SUCCESS) {
        printf("Unable to create semaphores. Result code: %s\n", string_VkResult(result));
        goto shutdown;
    }

    while (!PLATFORM_window_poll(g_window)) {
        bool resized = PLATFORM_window_resized(g_window);
        if (resized) {
            CpdSize size = PLATFORM_get_window_size(g_window);
            
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

        bool should_wait_for_swapchain_image = false;
        renderer->swapchain.current_image = RENDERER_acquire_next_image(renderer, &should_wait_for_swapchain_image);
        if (renderer->swapchain.current_image == UINT32_MAX) {
            goto shutdown;
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

        if (should_wait_for_swapchain_image) {
            result = renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence, VK_FALSE, UINT64_MAX);
            if (result != VK_SUCCESS) {
                printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
                goto shutdown;
            }
        }

        SWAPCHAIN_set_layout(&renderer->swapchain, &renderer->render_device, buffer,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        begin_rendering(renderer, buffer);

        renderer->render_device.vkCmdEndRenderingKHR(buffer);

        SWAPCHAIN_set_layout(&renderer->swapchain, &renderer->render_device, buffer,
            VK_PIPELINE_STAGE_2_NONE_KHR,
            VK_ACCESS_2_NONE_KHR,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

        VkSemaphoreSubmitInfoKHR signal_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
            .pNext = VK_NULL_HANDLE,
            .semaphore = render_semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
            .deviceIndex = 0
        };

        VkSubmitInfo2KHR submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .waitSemaphoreInfoCount = 0,
            .pWaitSemaphoreInfos = VK_NULL_HANDLE,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &buffer_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_info
        };

        result = renderer->render_device.vkResetFences(renderer->render_device.handle, 1, &render_fence);
        if (result != VK_SUCCESS) {
            printf("Unable to reset render fence. Result code: %s\n", string_VkResult(result));
        }

        result = renderer->render_device.vkQueueSubmit2KHR(renderer->render_device.graphics_family.queue, 1, &submit_info, render_fence);
        if (result != VK_SUCCESS) {
            printf("Unable to queue work. Result code: %s\n", string_VkResult(result));
        }

        renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &render_fence, VK_TRUE, UINT64_MAX);

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = VK_NULL_HANDLE,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
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
    renderer->render_device.vkDestroySemaphore(renderer->render_device.handle, acquire_semaphore, VK_NULL_HANDLE);
    renderer->render_device.vkDestroySemaphore(renderer->render_device.handle, render_semaphore, VK_NULL_HANDLE);

    renderer->render_device.vkFreeCommandBuffers(renderer->render_device.handle, renderer->render_device.graphics_family.pool, 1, &buffer);

    renderer->render_device.vkDestroyFence(renderer->render_device.handle, render_fence, VK_NULL_HANDLE);

    SWAPCHAIN_destroy(&renderer->swapchain, &renderer->render_device);
    destroy_renderer(renderer);
    vkDestroyInstance(g_instance, VK_NULL_HANDLE);

    PLATFORM_window_destroy(g_window);
    PLATFORM_free_vulkan_lib();
}

static void begin_rendering(CpdRenderer* renderer, VkCommandBuffer buffer) {
    CpdImage* image = &renderer->swapchain.images[renderer->swapchain.current_image];

    VkRenderingAttachmentInfoKHR attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = image->view,
        .imageLayout = image->layout,
        .resolveMode = VK_RESOLVE_MODE_NONE_KHR,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color.float32[0] = 1.0f,
        .clearValue.color.float32[1] = 0,
        .clearValue.color.float32[2] = 0,
        .clearValue.color.float32[3] = 1.0f
    };
    
    VkRenderingInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea.offset = (VkOffset2D){ 0, 0 },
        .renderArea.extent.width = renderer->swapchain.size.width,
        .renderArea.extent.height = renderer->swapchain.size.height,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment
    };

    renderer->render_device.vkCmdBeginRenderingKHR(buffer, &info);
}

static VkResult create_renderer(CpdRenderer** renderer) {
    *renderer = RENDERER_create(g_instance);
    VkResult result = RENDERER_select_render_device(*renderer);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = RENDERER_select_ui_device(*renderer);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSurfaceKHR surface;
    result = PLATFORM_create_surface(g_instance, g_window, &surface);
    if (result != VK_SUCCESS) {
        return result;
    }

    return SURFACE_initialize(&(*renderer)->surface, &(*renderer)->render_device, surface);
}

static VkResult create_semaphores(CpdRenderer* renderer, VkSemaphore* acquire, VkSemaphore* render) {
    VkResult result = create_semaphore(&renderer->render_device, acquire);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = create_semaphore(&renderer->render_device, render);
    if (result != VK_SUCCESS) {
        renderer->render_device.vkDestroySemaphore(renderer->render_device.handle, *acquire, VK_NULL_HANDLE);
    }

    return result;
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

    renderer->render_device.vkDestroyFence(renderer->render_device.handle, renderer->swapchain_image_fence, VK_NULL_HANDLE);

    RENDERER_destroy(renderer);
    vkDestroySurfaceKHR(g_instance, surface, VK_NULL_HANDLE);
}

static void load_global_pointers() {
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkCreateInstance);
    GET_INSTANCE_PROC_ADDR(VK_NULL_HANDLE, vkEnumerateInstanceExtensionProperties);
}

static void load_instance_pointers() {
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

static VkResult create_instance() {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = 0,
        .pApplicationName = "Cpeed",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = 0,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION_1_0
    };

    const CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_instance_extensions();
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

    if (result == VK_SUCCESS) {
        load_instance_pointers();
    }

    return result;
}
