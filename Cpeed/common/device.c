#include <stdio.h>
#include <malloc.h>

#include "device.h"
#include "device.queue_init.h"

#define GET_DEVICE_PROC_ADDR(cpeed_device, name) cpeed_device->name = (PFN_ ## name)vkGetDeviceProcAddr(cpeed_device->handle, #name)

static void init_device_functions(CpdDevice* cpeed_device) {
    GET_DEVICE_PROC_ADDR(cpeed_device, vkGetDeviceQueue);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyDevice);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkAcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkGetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkQueuePresentKHR);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateSemaphore);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroySemaphore);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateFence);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyFence);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkResetFences);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkWaitForFences);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkQueueWaitIdle);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkQueueSubmit2KHR);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateImageView);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyImageView);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateRenderPass);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyRenderPass);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateGraphicsPipelines);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateComputePipelines);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyPipeline);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateFramebuffer);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyFramebuffer);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateCommandPool);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyCommandPool);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkAllocateCommandBuffers);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkFreeCommandBuffers);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkResetCommandPool);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdBeginRenderingKHR);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdEndRenderingKHR);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkBeginCommandBuffer);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkEndCommandBuffer);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdBindPipeline);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdClearColorImage);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdPipelineBarrier2KHR);
}

void DEVICE_destroy(CpdDevice* cpeed_device) {
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->graphics_family.pool, VK_NULL_HANDLE);
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->compute_family.pool, VK_NULL_HANDLE);
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->transfer_family.pool, VK_NULL_HANDLE);

    cpeed_device->vkDestroyDevice(cpeed_device->handle, VK_NULL_HANDLE);
    free(cpeed_device->transfer_family.queues);
}

static VkResult init_device_pool(CpdDevice* cpeed_device, CpdQueueFamily* queue_family) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .queueFamilyIndex = queue_family->index
    };

    return cpeed_device->vkCreateCommandPool(cpeed_device->handle, &create_info, VK_NULL_HANDLE, &queue_family->pool);
}

static VkResult init_device_transfer_pool(CpdDevice* cpeed_device, CpdTransferQueueFamily* queue_family) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .queueFamilyIndex = queue_family->index
    };

    return cpeed_device->vkCreateCommandPool(cpeed_device->handle, &create_info, VK_NULL_HANDLE, &queue_family->pool);
}

static VkResult create_logical_device(
    VkPhysicalDevice physical, const CpdPlatformExtensions* extensions,
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t transfer_offset, VkDevice* device
) {
    uint32_t queue_create_info_count = 0;
    VkDeviceQueueCreateInfo* queue_create_info = (VkDeviceQueueCreateInfo*)0;
    if (!allocate_queue_create_infos(graphics, compute, transfer, transfer_count + transfer_offset, &queue_create_info_count, &queue_create_info)) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .pNext = VK_NULL_HANDLE,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
        .pNext = &dynamic_rendering_features,
        .synchronization2 = VK_TRUE
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &synchronization2_features,
        .flags = 0,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = VK_NULL_HANDLE,
        .enabledExtensionCount = extensions->count,
        .ppEnabledExtensionNames = extensions->count > 0 ? extensions->extensions : VK_NULL_HANDLE,
        .pEnabledFeatures = VK_NULL_HANDLE
    };

    VkResult result = vkCreateDevice(physical, &create_info, VK_NULL_HANDLE, device);
    destroy_queue_create_infos(queue_create_info, queue_create_info_count);
    return result;
}

VkResult DEVICE_initialize(
    CpdDevice* cpeed_device, VkPhysicalDevice physical, const CpdPlatformExtensions* extensions,
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t transfer_offset
) {
    VkDevice device;
    VkResult result = create_logical_device(physical, extensions, graphics, compute, transfer, transfer_count, transfer_offset, &device);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->physical_handle = physical;
    cpeed_device->handle = device;

    cpeed_device->graphics_family.index = graphics;
    cpeed_device->compute_family.index = compute;
    cpeed_device->transfer_family.index = transfer;
    cpeed_device->transfer_family.queue_count = transfer_count;

    init_device_functions(cpeed_device);

    cpeed_device->vkGetDeviceQueue(device, cpeed_device->graphics_family.index, 0, &cpeed_device->graphics_family.queue);
    result = init_device_pool(cpeed_device, &cpeed_device->graphics_family);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->vkGetDeviceQueue(device, cpeed_device->compute_family.index, 0, &cpeed_device->compute_family.queue);
    result = init_device_pool(cpeed_device, &cpeed_device->compute_family);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->transfer_family.queues = (CpdTransferQueue*)malloc(cpeed_device->transfer_family.queue_count * sizeof(CpdTransferQueue));
    if (cpeed_device->transfer_family.queues == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical, &physical_device_properties);
    printf("Initializing %s with %d transfer queue(s) and name \"%s\"\n",
        string_VkPhysicalDeviceType(physical_device_properties.deviceType),
        cpeed_device->transfer_family.queue_count,
        physical_device_properties.deviceName);

    for (uint32_t i = 0; i < cpeed_device->transfer_family.queue_count; i++) {
        cpeed_device->transfer_family.queues[i].bytes_queued = 0;
        cpeed_device->vkGetDeviceQueue(device, cpeed_device->transfer_family.index, i + transfer_offset, &cpeed_device->transfer_family.queues[i].handle);
    }

    return init_device_transfer_pool(cpeed_device, &cpeed_device->transfer_family);
}

VkResult DEVICE_reset_pools(CpdDevice* cpeed_device, VkCommandPoolResetFlags flags, bool reset_transfer) {
    VkResult result = cpeed_device->vkResetCommandPool(cpeed_device->handle, cpeed_device->graphics_family.pool, flags);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = cpeed_device->vkResetCommandPool(cpeed_device->handle, cpeed_device->compute_family.pool, flags);
    if (result != VK_SUCCESS) {
        return result;
    }

    if (reset_transfer) {
        result = cpeed_device->vkResetCommandPool(cpeed_device->handle, cpeed_device->transfer_family.pool, flags);
    }

    return result;
}

VkResult DEVICE_wait_idle(CpdDevice* cpeed_device, bool wait_transfer) {
    VkResult result = cpeed_device->vkQueueWaitIdle(cpeed_device->graphics_family.queue);
    if (result != VK_SUCCESS) {
        return result;
    }

    result = cpeed_device->vkQueueWaitIdle(cpeed_device->compute_family.queue);
    if (result != VK_SUCCESS) {
        return result;
    }

    for (uint32_t i = 0; i < cpeed_device->transfer_family.queue_count && wait_transfer; i++) {
        if (cpeed_device->transfer_family.queues[i].bytes_queued == 0) {
            continue;
        }

        result = cpeed_device->vkQueueWaitIdle(cpeed_device->transfer_family.queues[i].handle);
        cpeed_device->transfer_family.queues[i].bytes_queued = 0;

        if (result != VK_SUCCESS) {
            break;
        }
    }

    return result;
}
