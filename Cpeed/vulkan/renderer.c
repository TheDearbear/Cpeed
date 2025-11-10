#include <malloc.h>

#include "renderer.h"
#include "../platform.h"

typedef bool (*CpdDeviceInitializer)(CpdRenderer*, VkPhysicalDevice, VkResult*);

CpdRenderer* RENDERER_create(CpdRendererInitParams* params) {
    CpdRenderer* renderer = (CpdRenderer*)malloc(sizeof(CpdRenderer));
    if (renderer == 0) {
        return 0;
    }

    CpdRenderSettings* render_settings = (CpdRenderSettings*)malloc(sizeof(CpdRenderSettings));
    if (render_settings == 0) {
        free(renderer);
        return 0;
    }

    CpdFrame* frame = (CpdFrame*)malloc(sizeof(CpdFrame));
    if (frame == 0) {
        free(render_settings);
        free(renderer);
        return 0;
    }

    render_settings->allow_render = true;
    render_settings->force_disable_render = false;

    frame->background = params->background;

    renderer->instance = params->instance;
    renderer->api_version = params->max_api_version;
    renderer->target_version = params->api_version;
    renderer->instance_extensions = *params->instance_extensions;
    renderer->creation_time = get_clock_usec();
    renderer->last_frame_end = renderer->creation_time;
    renderer->frame = frame;
    renderer->window = 0;
    renderer->surface.handle = VK_NULL_HANDLE;
    renderer->render_device.handle = VK_NULL_HANDLE;
    renderer->swapchain.handle = VK_NULL_HANDLE;
    renderer->swapchain.image_count = 0;
    renderer->swapchain_image_fence = VK_NULL_HANDLE;
    renderer->render_settings = render_settings;

    return renderer;
}

void RENDERER_destroy(CpdRenderer* renderer) {
    if (renderer->swapchain.handle != VK_NULL_HANDLE) {
        SWAPCHAIN_destroy(&renderer->swapchain, &renderer->render_device);
        renderer->swapchain.handle = VK_NULL_HANDLE;
    }

    if (renderer->render_device.handle != VK_NULL_HANDLE) {
        DEVICE_destroy(&renderer->render_device);
        renderer->render_device.handle = VK_NULL_HANDLE;
    }

    free(renderer->frame);
    free(renderer->render_settings);
    free(renderer);
}

static uint32_t get_physical_device_family(
    VkQueueFamilyProperties* properties, uint32_t count,
    VkQueueFlagBits include_flags, VkQueueFlagBits exclude_flags
) {
    uint32_t result = UINT32_MAX;

    for (uint32_t i = 0; i < count; i++) {
        VkQueueFlags flags = properties[i].queueFlags;

        if ((flags & include_flags) != 0) {
            if (result == UINT32_MAX) {
                result = i;
            }
            else if ((flags & exclude_flags) == 0) {
                result = i;
                break;
            }
        }
    }

    return result;
}

static void get_physical_device_families(VkPhysicalDevice physical,
    uint32_t* graphics, uint32_t* compute, uint32_t* transfer,
    uint32_t* transfer_count, uint32_t* transfer_offset
) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, VK_NULL_HANDLE);

    VkQueueFamilyProperties* properties = (VkQueueFamilyProperties*)malloc(count * sizeof(VkQueueFamilyProperties));
    if (properties == 0) {
        return;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, properties);

    *graphics = get_physical_device_family(properties, count, VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT);
    *compute = get_physical_device_family(properties, count, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
    *transfer = get_physical_device_family(properties, count, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    *transfer_offset = 0;

    if (*transfer != UINT32_MAX) {
        *transfer_count = properties[*transfer].queueCount;

        if (*transfer_count > 1 && (*transfer == *graphics || *transfer == *compute)) {
            *transfer_count -= 1;
            *transfer_offset += 1;
        }
    }
    else {
        *transfer_count = 0;
    }

    free(properties);
}

static uint32_t get_physical_device_family2(
    VkQueueFamilyProperties2* properties, uint32_t count,
    VkQueueFlagBits include_flags, VkQueueFlagBits exclude_flags
) {
    uint32_t result = UINT32_MAX;

    for (uint32_t i = 0; i < count; i++) {
        VkQueueFlags flags = properties[i].queueFamilyProperties.queueFlags;

        if ((flags & include_flags) != 0) {
            if (result == UINT32_MAX) {
                result = i;
            }
            else if ((flags & exclude_flags) == 0) {
                result = i;
                break;
            }
        }
    }

    return result;
}

static void get_physical_device_families2(VkPhysicalDevice physical,
    uint32_t* graphics, uint32_t* compute, uint32_t* transfer,
    uint32_t* transfer_count, uint32_t* transfer_offset
) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical, &count, VK_NULL_HANDLE);

    VkQueueFamilyProperties2* properties = (VkQueueFamilyProperties2*)malloc(count * sizeof(VkQueueFamilyProperties2));
    if (properties == 0) {
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        properties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        properties[i].pNext = 0;
    }

    vkGetPhysicalDeviceQueueFamilyProperties2(physical, &count, properties);

    *graphics = get_physical_device_family2(properties, count, VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT);
    *compute = get_physical_device_family2(properties, count, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
    *transfer = get_physical_device_family2(properties, count, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    *transfer_offset = 0;

    if (*transfer != UINT32_MAX) {
        *transfer_count = properties[*transfer].queueFamilyProperties.queueCount;

        if (*transfer_count > 1 && (*transfer == *graphics || *transfer == *compute)) {
            *transfer_count -= 1;
            *transfer_offset += 1;
        }
    }
    else {
        *transfer_count = 0;
    }

    free(properties);
}

static bool try_initialize_render_device(CpdRenderer* renderer, VkPhysicalDevice physical_device, VkResult* result) {
    uint32_t graphics_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;

    uint32_t transfer_queue_count;
    uint32_t transfer_queue_offset;
    
    if (vkGetPhysicalDeviceQueueFamilyProperties2 != VK_NULL_HANDLE) {
        get_physical_device_families2(physical_device,
            &graphics_family_index, &compute_family_index, &transfer_family_index,
            &transfer_queue_count, &transfer_queue_offset);
    }
    else {
        get_physical_device_families(physical_device,
            &graphics_family_index, &compute_family_index, &transfer_family_index,
            &transfer_queue_count, &transfer_queue_offset);
    }

    CpdRenderDeviceVulkanExtensions* device_extensions = (CpdRenderDeviceVulkanExtensions*)malloc(sizeof(CpdRenderDeviceVulkanExtensions));
    if (device_extensions == 0) {
        return false;
    }

    initialize_vulkan_render_device_extensions(device_extensions);

    if (graphics_family_index != UINT32_MAX && compute_family_index != UINT32_MAX && transfer_family_index != UINT32_MAX) {
        CpdDeviceInitParams init_params = {
            .physical_device = physical_device,
            .device_extensions = (CpdVulkanExtension*)device_extensions,
            .device_extension_count = GET_EXTENSIONS_COUNT(CpdRenderDeviceVulkanExtensions),
            .graphics_family = graphics_family_index,
            .compute_family = compute_family_index,
            .transfer_family = transfer_family_index,
            .transfer_count = transfer_queue_count,
            .transfer_offset = transfer_queue_offset
        };

        *result = DEVICE_initialize(&renderer->render_device, &init_params);
        return true;
    }

    return false;
}

static VkResult device_selector(CpdRenderer* renderer,
    VkPhysicalDeviceType targetType, VkPhysicalDeviceType secondaryType,
    CpdDeviceInitializer device_initializer
) {
    VkPhysicalDevice* devices;
    uint32_t count;

    VkResult result = vkEnumeratePhysicalDevices(renderer->instance, &count, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        return result;
    }

    devices = (VkPhysicalDevice*)malloc(count * sizeof(VkPhysicalDevice));
    if (devices == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkEnumeratePhysicalDevices(renderer->instance, &count, devices);
    if (result != VK_SUCCESS) {
        free(devices);
        return result;
    }

    uint32_t select_index = UINT32_MAX;
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties2 properties = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = 0
        };
        get_physical_device_properties(devices[i], &properties);

        if (properties.properties.deviceType == targetType) {
            select_index = i;
            break;
        }

        if (properties.properties.deviceType == secondaryType && select_index == UINT32_MAX) {
            select_index = i;
        }
    }

    if (select_index != UINT32_MAX) {
        if (device_initializer(renderer, devices[select_index], &result)) {
            free(devices);
            return result;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == select_index) {
            continue;
        }

        if (device_initializer(renderer, devices[i], &result)) {
            free(devices);
            return result;
        }
    }

    free(devices);
    return VK_ERROR_UNKNOWN;
}

VkResult RENDERER_select_render_device(CpdRenderer* renderer) {
    return device_selector(renderer,
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
        try_initialize_render_device);
}

VkResult RENDERER_reset_pools(CpdRenderer* renderer) {
    return DEVICE_reset_pools(&renderer->render_device, 0, false);
}

VkResult RENDERER_wait_idle(CpdRenderer* renderer) {
    return DEVICE_wait_idle(&renderer->render_device, false);
}

VkResult RENDERER_acquire_next_image(CpdRenderer* renderer, bool wait_for_fence) {
    VkResult result = renderer->render_device.vkResetFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = renderer->render_device.vkAcquireNextImageKHR(
        renderer->render_device.handle,
        renderer->swapchain.handle,
        UINT64_MAX,
        VK_NULL_HANDLE,
        renderer->swapchain_image_fence,
        &renderer->swapchain.current_image);

    if ((result != VK_SUCCESS && result != VK_NOT_READY && result != VK_SUBOPTIMAL_KHR) || (result == VK_NOT_READY && !wait_for_fence)) {
        return result;
    }

    return renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence, VK_FALSE, UINT64_MAX);
}
