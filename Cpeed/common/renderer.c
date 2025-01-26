#include "renderer.h"

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
        free(renderer->render_device.transfer_queues);

        renderer->render_device.handle = VK_NULL_HANDLE;
    }

    if (renderer->ui_device.handle != VK_NULL_HANDLE) {
        renderer->ui_device.vkDestroyDevice(renderer->ui_device.handle, VK_NULL_HANDLE);
        free(renderer->ui_device.transfer_queues);

        renderer->ui_device.handle = VK_NULL_HANDLE;
    }

    free(renderer);
}

static VkResult init_device(
    CpdDevice* cpeed_device, VkPhysicalDevice physical, CpdPlatformExtensions* extensions,
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t transfer_offset
) {

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical, &physical_device_properties);
    printf("Initializing %s with name: %s\n", string_VkPhysicalDeviceType(physical_device_properties.deviceType), physical_device_properties.deviceName);

    uint32_t queue_create_info_count = 0;
    VkDeviceQueueCreateInfo* queue_create_info = (VkDeviceQueueCreateInfo*)0;
    if (!allocate_queue_create_infos(graphics, compute, transfer, transfer_count + transfer_offset, &queue_create_info_count, &queue_create_info)) {
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
    cpeed_device->transfer_queue_count = transfer_count;

    cpeed_device->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
    cpeed_device->vkDestroyDevice = (PFN_vkDestroyDevice)vkGetDeviceProcAddr(device, "vkDestroyDevice");

    cpeed_device->vkGetDeviceQueue(device, cpeed_device->graphics_index, 0, &cpeed_device->graphics_queue);
    cpeed_device->vkGetDeviceQueue(device, cpeed_device->compute_index, 0, &cpeed_device->compute_queue);

    cpeed_device->transfer_queues = (CpdTransferQueue*)malloc(cpeed_device->transfer_queue_count * sizeof(CpdTransferQueue));
    if (cpeed_device->transfer_queues == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    printf("Initializing device with %d transfer queue(s)\n", cpeed_device->transfer_queue_count);

    for (uint32_t i = 0; i < cpeed_device->transfer_queue_count; i++) {
        cpeed_device->transfer_queues[i].bytes_queued = 0;
        cpeed_device->vkGetDeviceQueue(device, cpeed_device->transfer_index, i + transfer_offset, &cpeed_device->transfer_queues[i].handle);
    }

    return VK_SUCCESS;
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
            printf("Enabling render device extension: %s", extensions->extensions[i]);
        }

        *result = init_device(cpeed_device, physical_device, extensions,
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
            printf("Enabling ui device extension: %s", extensions->extensions[i]);
        }

        *result = init_device(cpeed_device, physical_device, extensions,
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
