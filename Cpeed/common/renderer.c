#include <stdio.h>
#include <malloc.h>

#include "renderer.h"
#include "../platform.h"

CpdRenderer* RENDERER_create(VkInstance instance) {
    CpdRenderer* renderer = (CpdRenderer*)malloc(sizeof(CpdRenderer));
    if (renderer == 0) {
        return 0;
    }

    renderer->instance = instance;
    renderer->surface.handle = VK_NULL_HANDLE;
    renderer->render_device.handle = VK_NULL_HANDLE;
    renderer->ui_device.handle = VK_NULL_HANDLE;
    renderer->swapchain.handle = VK_NULL_HANDLE;
    renderer->swapchain.image_count = 0;

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

    if (renderer->ui_device.handle != VK_NULL_HANDLE) {
        DEVICE_destroy(&renderer->ui_device);
        renderer->ui_device.handle = VK_NULL_HANDLE;
    }

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

static bool try_initialize_render_device(VkPhysicalDevice physical_device, CpdDevice* cpeed_device, VkResult* result) {
    uint32_t graphics_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;

    uint32_t transfer_queue_count;
    uint32_t transfer_queue_offset;
    
    get_physical_device_families(physical_device,
        &graphics_family_index, &compute_family_index, &transfer_family_index,
        &transfer_queue_count, &transfer_queue_offset);

    if (graphics_family_index != UINT32_MAX && compute_family_index != UINT32_MAX && transfer_family_index != UINT32_MAX) {
        CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_render_device_extensions();

        for (unsigned int i = 0; i < extensions->count; i++) {
            printf("Enabling render device extension: %s\n", extensions->extensions[i]);
        }

        *result = DEVICE_initialize(cpeed_device, physical_device, extensions,
            graphics_family_index, compute_family_index, transfer_family_index,
            transfer_queue_count, transfer_queue_offset);

        PLATFORM_free_vulkan_extensions(extensions);
        return true;
    }

    return false;
}

static bool try_initialize_ui_device(VkPhysicalDevice physical_device, CpdDevice* cpeed_device, VkResult* result) {
    uint32_t graphics_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;

    uint32_t transfer_queue_count;
    uint32_t transfer_queue_offset;

    get_physical_device_families(physical_device,
        &graphics_family_index, &compute_family_index, &transfer_family_index,
        &transfer_queue_count, &transfer_queue_offset);

    if (graphics_family_index != UINT32_MAX && compute_family_index != UINT32_MAX && transfer_family_index != UINT32_MAX) {
        CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_ui_device_extensions();

        for (unsigned int i = 0; i < extensions->count; i++) {
            printf("Enabling ui device extension: %s\n", extensions->extensions[i]);
        }

        *result = DEVICE_initialize(cpeed_device, physical_device, extensions,
            graphics_family_index, compute_family_index, transfer_family_index,
            transfer_queue_count, transfer_queue_offset);

        PLATFORM_free_vulkan_extensions(extensions);
        return true;
    }

    return false;
}

VkResult RENDERER_select_render_device(CpdRenderer* renderer) {
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

    uint32_t performance_device_index = UINT32_MAX;
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            performance_device_index = i;
            break;
        }

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU && performance_device_index == UINT32_MAX) {
            performance_device_index = i;
        }
    }

    if (performance_device_index != UINT32_MAX) {
        if (try_initialize_render_device(devices[performance_device_index], &renderer->render_device, &result)) {
            free(devices);
            return result;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == performance_device_index) {
            continue;
        }

        if (try_initialize_render_device(devices[i], &renderer->render_device, &result)) {
            free(devices);
            return result;
        }
    }

    free(devices);
    return VK_ERROR_UNKNOWN;
}

VkResult RENDERER_select_ui_device(CpdRenderer* renderer) {
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

    uint32_t iGPU_index = UINT32_MAX;
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            iGPU_index = i;
            break;
        }
    }

    if (iGPU_index != UINT32_MAX) {
        if (try_initialize_ui_device(devices[iGPU_index], &renderer->ui_device, &result)) {
            free(devices);
            return result;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == iGPU_index) {
            continue;
        }

        if (try_initialize_ui_device(devices[i], &renderer->ui_device, &result)) {
            free(devices);
            return result;
        }
    }

    free(devices);
    return VK_ERROR_UNKNOWN;
}

VkResult RENDERER_reset_pools(CpdRenderer* renderer) {
    VkResult result = DEVICE_reset_pools(&renderer->render_device, 0, false);
    if (result != VK_SUCCESS) {
        return result;
    }

    return DEVICE_reset_pools(&renderer->ui_device, 0, false);
}

VkResult RENDERER_wait_idle(CpdRenderer* renderer) {
    VkResult result = DEVICE_wait_idle(&renderer->render_device, false);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = DEVICE_wait_idle(&renderer->ui_device, false);

    return result;
}

uint32_t RENDERER_acquire_next_image(CpdRenderer* renderer) {
    VkResult result = renderer->render_device.vkResetFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence);
    if (result != VK_SUCCESS) {
        printf("Unable to reset swapchain image fence. Result code: %s\n", string_VkResult(result));
        return UINT32_MAX;
    }

    uint32_t index = 0;
    result = renderer->render_device.vkAcquireNextImageKHR(
        renderer->render_device.handle,
        renderer->swapchain.handle,
        UINT64_MAX,
        VK_NULL_HANDLE,
        renderer->swapchain_image_fence,
        &index);

    if (result == VK_TIMEOUT) {
        printf("Swapchain timed out\n");
    }
    else if (result != VK_SUCCESS) {
        printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
        return UINT32_MAX;
    }

    result = renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence, VK_TRUE, UINT64_MAX);
    if (result == VK_TIMEOUT) {
        printf("Swapchain timed out\n");
    }
    else if (result != VK_SUCCESS) {
        printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
        return UINT32_MAX;
    }

    return index;
}
