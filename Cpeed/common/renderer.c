#include <stdio.h>
#include <malloc.h>

#include "renderer.h"
#include "../platform.h"

typedef bool (*CpdDeviceInitializer)(VkPhysicalDevice, CpdDevice*, VkResult*);

static const CpdPlatformExtensions* alloc_render_device_extensions();
static const CpdPlatformExtensions* alloc_ui_device_extensions();
static void free_device_extensions(const CpdPlatformExtensions* extensions);

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
        const CpdPlatformExtensions* extensions = alloc_render_device_extensions();

        for (unsigned int i = 0; i < extensions->count; i++) {
            printf("Enabling render device extension: %s\n", extensions->extensions[i]);
        }

        *result = DEVICE_initialize(cpeed_device, physical_device, extensions,
            graphics_family_index, compute_family_index, transfer_family_index,
            transfer_queue_count, transfer_queue_offset);

        free_device_extensions(extensions);
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
        const CpdPlatformExtensions* extensions = alloc_ui_device_extensions();

        for (unsigned int i = 0; i < extensions->count; i++) {
            printf("Enabling ui device extension: %s\n", extensions->extensions[i]);
        }

        *result = DEVICE_initialize(cpeed_device, physical_device, extensions,
            graphics_family_index, compute_family_index, transfer_family_index,
            transfer_queue_count, transfer_queue_offset);

        free_device_extensions(extensions);
        return true;
    }

    return false;
}

static VkResult device_selector(CpdRenderer* renderer,
    VkPhysicalDeviceType targetType, VkPhysicalDeviceType secondaryType,
    CpdDevice* cpeed_device, CpdDeviceInitializer device_initializer
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
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        if (properties.deviceType == targetType) {
            select_index = i;
            break;
        }

        if (properties.deviceType == secondaryType && select_index == UINT32_MAX) {
            select_index = i;
        }
    }

    if (select_index != UINT32_MAX) {
        if (device_initializer(devices[select_index], cpeed_device, &result)) {
            free(devices);
            return result;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == select_index) {
            continue;
        }

        if (device_initializer(devices[i], cpeed_device, &result)) {
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
        &renderer->render_device, try_initialize_render_device);
}

VkResult RENDERER_select_ui_device(CpdRenderer* renderer) {
    return device_selector(renderer,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM,
        &renderer->ui_device, try_initialize_ui_device);
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

uint32_t RENDERER_acquire_next_image(CpdRenderer* renderer, bool* should_wait_for_fence) {
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

    if (result != VK_SUCCESS && result != VK_NOT_READY && result != VK_SUBOPTIMAL_KHR) {
        printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
        return UINT32_MAX;
    }

    if (should_wait_for_fence != 0) {
        *should_wait_for_fence = result == VK_NOT_READY;
    }

    if (result != VK_NOT_READY || should_wait_for_fence != 0) {
        return index;
    }

    result = renderer->render_device.vkWaitForFences(renderer->render_device.handle, 1, &renderer->swapchain_image_fence, VK_FALSE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
        return UINT32_MAX;
    }

    return index;
}

static const CpdPlatformExtensions* alloc_render_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    const int extension_count = 3;

    const char** extensionNames = (const char**)malloc(extension_count * sizeof(char*));
    if (extensionNames == 0) {
        free(extensions);
        return 0;
    }

    extensions->count = extension_count;
    extensions->extensions = extensionNames;

    extensionNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    extensionNames[1] = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
    extensionNames[2] = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;

    return extensions;
}

static const CpdPlatformExtensions* alloc_ui_device_extensions() {
    CpdPlatformExtensions* extensions = (CpdPlatformExtensions*)malloc(sizeof(CpdPlatformExtensions));
    if (extensions == 0) {
        return 0;
    }

    extensions->count = 0;

    return extensions;
}

static void free_device_extensions(const CpdPlatformExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free((void*)extensions);
}
