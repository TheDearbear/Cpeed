#include "renderer.h" 
#include <malloc.h>

CpdRenderer* RENDERER_create(VkInstance instance) {
    CpdRenderer* renderer = (CpdRenderer*)malloc(sizeof(CpdRenderer));
    if (renderer == 0) {
        return 0;
    }

    renderer->instance = instance;
    renderer->render_device.handle = VK_NULL_HANDLE;
    renderer->ui_device.handle = VK_NULL_HANDLE;

    return renderer;
}

void RENDERER_destroy(CpdRenderer* renderer) {
    if (renderer->render_device.handle != VK_NULL_HANDLE) {
        renderer->render_device.vkDestroyDevice(renderer->render_device.handle, VK_NULL_HANDLE);
        renderer->render_device.handle = VK_NULL_HANDLE;
    }

    if (renderer->ui_device.handle != VK_NULL_HANDLE) {
        renderer->ui_device.vkDestroyDevice(renderer->ui_device.handle, VK_NULL_HANDLE);
        renderer->ui_device.handle = VK_NULL_HANDLE;
    }

    free(renderer);
}

void get_physical_device_families(VkPhysicalDevice physical, uint32_t* graphics, uint32_t* compute, uint32_t* transfer) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, VK_NULL_HANDLE);

    VkQueueFamilyProperties* properties = (VkQueueFamilyProperties*)malloc(count * sizeof(VkQueueFamilyProperties));
    if (properties == 0) {
        return;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, properties);

    *graphics = UINT32_MAX;
    *compute = UINT32_MAX;
    *transfer = UINT32_MAX;

    for (uint32_t i = 0; i < count; i++) {
        VkQueueFlags flags = properties[i].queueFlags;

        if (flags & VK_QUEUE_TRANSFER_BIT != 0 && *transfer == UINT32_MAX) {
            *transfer = i;
        }

        if (flags & VK_QUEUE_GRAPHICS_BIT != 0 && (*graphics == UINT32_MAX || *graphics != *compute)) {
            if (*graphics == UINT32_MAX || (flags & VK_QUEUE_COMPUTE_BIT != 0 && *graphics != *compute)) {
                *graphics = i;
            }

            if (flags & VK_QUEUE_COMPUTE_BIT != 0) {
                *compute = i;
            }
        }

        if (flags & VK_QUEUE_COMPUTE_BIT != 0 && *compute == UINT32_MAX) {
            *compute = i;
        }
    }

    free(properties);
}


static bool allocate_queue_create_infos(uint32_t graphics, uint32_t compute, uint32_t transfer, uint32_t* count, VkDeviceQueueCreateInfo** infos);
static void destroy_queue_create_infos(VkDeviceQueueCreateInfo* infos, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        free(infos[i].pQueuePriorities);
    }

    free(infos);
}

static VkResult init_device(
    CpdDevice* cpeed_device, VkPhysicalDevice physical, CpdPlatformExtensions* extensions,
    uint32_t graphics, uint32_t compute, uint32_t transfer
) {

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical, &physical_device_properties);
    printf("Initializing %s with name: %s\n", string_VkPhysicalDeviceType(physical_device_properties.deviceType), physical_device_properties.deviceName);

    uint32_t queue_create_info_count;
    VkDeviceQueueCreateInfo* queue_create_info;
    if (!allocate_queue_create_infos(graphics, compute, transfer, &queue_create_info_count, &queue_create_info)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = extensions->count,
        .pEnabledFeatures = extensions->count > 0 ? extensions->extensions : 0
    };

    VkDevice device;
    VkResult result = vkCreateDevice(physical, &create_info, VK_NULL_HANDLE, &device);
    destroy_queue_create_infos(queue_create_info, queue_create_info_count);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->physical_handle = physical;
    cpeed_device->handle = device;

    cpeed_device->graphics_index = graphics;
    cpeed_device->compute_index = compute;
    cpeed_device->transfer_index = transfer;

    cpeed_device->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
    cpeed_device->vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");

    cpeed_device->vkGetDeviceQueue(device, cpeed_device->graphics_index, 0, &cpeed_device->graphics_queue);
    cpeed_device->vkGetDeviceQueue(device, cpeed_device->compute_index, 0, &cpeed_device->compute_queue);
    cpeed_device->vkGetDeviceQueue(device, cpeed_device->transfer_index, 0, &cpeed_device->transfer_queue);

    return VK_SUCCESS;
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

    uint32_t graphics_family_index;
    uint32_t compute_family_index;
    uint32_t transfer_family_index;

    if (performance_device_index != UINT32_MAX) {
        VkPhysicalDevice device = devices[performance_device_index];

        get_physical_device_families(device, &graphics_family_index, &compute_family_index, &transfer_family_index);

        if (graphics_family_index != UINT32_MAX && compute_family_index != UINT32_MAX && transfer_family_index != UINT32_MAX) {
            free(devices);

            CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_render_device_extensions();

            VkResult result = init_device(&renderer->render_device, device, extensions, graphics_family_index, compute_family_index, transfer_family_index);

            PLATFORM_free_vulkan_extensions(extensions);
            return result;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == performance_device_index) {
            continue;
        }

        VkPhysicalDevice device = devices[i];

        get_physical_device_families(device, &graphics_family_index, &compute_family_index, &transfer_family_index);

        if (graphics_family_index != UINT32_MAX && compute_family_index != UINT32_MAX && transfer_family_index != UINT32_MAX) {
            free(devices);

            CpdPlatformExtensions* extensions = PLATFORM_alloc_vulkan_render_device_extensions();

            VkResult result = init_device(&renderer->render_device, device, extensions, graphics_family_index, compute_family_index, transfer_family_index);

            PLATFORM_free_vulkan_extensions(extensions);
            return result;
        }
    }

    free(devices);
    return VK_ERROR_UNKNOWN;
}

VkResult RENDERER_select_ui_device(CpdRenderer* renderer) {
    //
}

static bool allocate_queue_create_infos(uint32_t graphics, uint32_t compute, uint32_t transfer, uint32_t* count, VkDeviceQueueCreateInfo** infos) {
    VkDeviceQueueCreateInfo local_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
    };
    
    if (graphics == compute && graphics == transfer) {
        float* priorities = (float*)malloc(3 * sizeof(float));
        if (priorities == 0) {
            return false;
        }

        priorities[0] = 1.0f;
        priorities[1] = 1.0f;
        priorities[2] = 1.0f;

        VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*)malloc(1 * sizeof(VkDeviceQueueCreateInfo));
        if (info == 0) {
            free(priorities);
            return false;
        }

        local_info.queueFamilyIndex = graphics;
        local_info.queueCount = 3;
        local_info.pQueuePriorities = priorities;
        info[0] = local_info;

        *count = 1;
        *infos = info;

        return true;
    }
    else if (graphics == compute || graphics == transfer || compute == transfer) {
        float* first_priorities = (float*)malloc(2 * sizeof(float));
        if (first_priorities == 0) {
            return false;
        }

        first_priorities[0] = 1.0f;
        first_priorities[1] = 1.0f;

        float* second_priorities = (float*)malloc(1 * sizeof(float));
        if (second_priorities == 0) {
            free(first_priorities);
            return false;
        }

        second_priorities[0] = 1.0f;

        VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*)malloc(2 * sizeof(VkDeviceQueueCreateInfo));
        if (info == 0) {
            free(first_priorities);
            free(second_priorities);
            return false;
        }

        local_info.queueFamilyIndex = compute != transfer ? graphics : compute;
        local_info.queueCount = 2;
        local_info.pQueuePriorities = first_priorities;
        info[0] = local_info;

        local_info.queueFamilyIndex = compute == transfer ? graphics : compute;
        local_info.queueCount = 1;
        local_info.pQueuePriorities = second_priorities;
        info[1] = local_info;

        *count = 2;
        *infos = info;

        return true;
    }

    float* graphics_priorities = (float*)malloc(1 * sizeof(float));
    if (graphics_priorities == 0) {
        return false;
    }

    float* compute_priorities = (float*)malloc(1 * sizeof(float));
    if (compute_priorities == 0) {
        free(graphics_priorities);
        return false;
    }

    float* transfer_priorities = (float*)malloc(1 * sizeof(float));
    if (transfer_priorities == 0) {
        free(graphics_priorities);
        free(compute_priorities);
        return false;
    }

    graphics_priorities[0] = 1.0f;
    compute_priorities[0] = 1.0f;
    transfer_priorities[0] = 1.0f;

    VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*)malloc(3 * sizeof(VkDeviceQueueCreateInfo));
    if (info == false) {
        free(graphics_priorities);
        free(compute_priorities);
        free(transfer_priorities);
        return false;
    }

    local_info.queueFamilyIndex = graphics;
    local_info.queueCount = 1;
    local_info.pQueuePriorities = graphics_priorities;
    info[0] = local_info;

    local_info.queueFamilyIndex = compute;
    local_info.queueCount = 1;
    local_info.pQueuePriorities = compute_priorities;
    info[1] = local_info;

    local_info.queueFamilyIndex = transfer;
    local_info.queueCount = 1;
    local_info.pQueuePriorities = transfer_priorities;
    info[2] = local_info;

    *count = 3;
    *infos = info;

    return true;
}
