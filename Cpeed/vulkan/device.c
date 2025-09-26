#include <string.h>
#include <malloc.h>

#include "../platform/backend/vulkan.h"
#include "../platform/logging.h"
#include "device.h"
#include "device.queue_init.h"

#define GET_DEVICE_PROC_ADDR(cpeed_device, name) cpeed_device->name = (PFN_ ## name)vkGetDeviceProcAddr(cpeed_device->handle, #name)
#define GET_DEVICE_PROC_ADDR_SUFFIX(cpeed_device, name, suffix) cpeed_device->name = (PFN_ ## name)vkGetDeviceProcAddr(cpeed_device->handle, #name suffix)

#define GET_DEVICE_PROC_ADDR_EXTENSION(cpeed_device, name, suffix, extension) { \
    CpdVulkanExtensionLoadMethod load_method = get_extension_load_method(cpeed_device, extension); \
    if (load_method == CpdVulkanExtensionLoadMethod_FromCore) { GET_DEVICE_PROC_ADDR(cpeed_device, name); } \
    else if (load_method == CpdVulkanExtensionLoadMethod_FromExtension) { GET_DEVICE_PROC_ADDR_SUFFIX(cpeed_device, name, suffix); } \
    else { cpeed_device->name = VK_NULL_HANDLE; } \
}

static CpdVulkanExtensionLoadMethod get_extension_load_method(CpdDevice* cpeed_device, const char* extension) {
    CpdVulkanExtensionLoadMethod method = CpdVulkanExtensionLoadMethod_NotLoaded;

    for (uint32_t i = 0; i < cpeed_device->extension_count; i++) {
        if (strcmp(cpeed_device->extensions[i].name, extension) != 0) {
            continue;
        }

        return cpeed_device->extensions[i].load_method;
    }

    return method;
}

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
    GET_DEVICE_PROC_ADDR_EXTENSION(cpeed_device, vkQueueSubmit2, "KHR", VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateImage);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyImage);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCreateImageView);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkDestroyImageView);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkAllocateMemory);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkFreeMemory);
    
    GET_DEVICE_PROC_ADDR(cpeed_device, vkMapMemory);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkUnmapMemory);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkBindBufferMemory);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkBindImageMemory);

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

    GET_DEVICE_PROC_ADDR_EXTENSION(cpeed_device, vkCmdBeginRendering, "KHR", VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    GET_DEVICE_PROC_ADDR_EXTENSION(cpeed_device, vkCmdEndRendering, "KHR", VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkBeginCommandBuffer);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkEndCommandBuffer);

    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdBindPipeline);
    GET_DEVICE_PROC_ADDR(cpeed_device, vkCmdClearColorImage);
    GET_DEVICE_PROC_ADDR_EXTENSION(cpeed_device, vkCmdPipelineBarrier2, "KHR", VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
}

void DEVICE_destroy(CpdDevice* cpeed_device) {
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->graphics_family.pool, VK_NULL_HANDLE);
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->compute_family.pool, VK_NULL_HANDLE);
    cpeed_device->vkDestroyCommandPool(cpeed_device->handle, cpeed_device->transfer_family.pool, VK_NULL_HANDLE);

    cpeed_device->vkDestroyDevice(cpeed_device->handle, VK_NULL_HANDLE);
    free(cpeed_device->extensions);
    free(cpeed_device->transfer_family.queues);
}

static VkResult init_device_pool(CpdDevice* cpeed_device, CpdQueueFamily* queue_family) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .queueFamilyIndex = queue_family->index
    };

    return cpeed_device->vkCreateCommandPool(cpeed_device->handle, &create_info, VK_NULL_HANDLE, &queue_family->pool);
}

static VkResult init_device_transfer_pool(CpdDevice* cpeed_device, CpdTransferQueueFamily* queue_family) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .queueFamilyIndex = queue_family->index
    };

    return cpeed_device->vkCreateCommandPool(cpeed_device->handle, &create_info, VK_NULL_HANDLE, &queue_family->pool);
}

static CpdVulkanExtensions* alloc_device_extensions(CpdDevice* cpeed_device, CpdDeviceInitParams* params) {
    uint32_t property_count;

    VkResult result = vkEnumerateDeviceExtensionProperties(params->physical_device, 0, &property_count, 0);
    if (result != VK_SUCCESS) {
        return 0;
    }

    VkExtensionProperties* properties = (VkExtensionProperties*)malloc(property_count * sizeof(VkExtensionProperties));
    if (properties == 0) {
        return 0;
    }

    result = vkEnumerateDeviceExtensionProperties(params->physical_device, 0, &property_count, properties);
    if (result != VK_SUCCESS) {
        return 0;
    }

    uint32_t extensions_found = 0;
    uint32_t loaded_from_extension = 0;

    for (uint32_t i = 0; i < property_count && extensions_found < cpeed_device->extension_count; i++) {
        for (uint32_t j = 0; j < cpeed_device->extension_count; j++) {
            CpdVulkanExtension* extension = &cpeed_device->extensions[j];

            if (strcmp(properties[i].extensionName, extension->name) != 0) {
                continue;
            }

            extensions_found++;

            if (extension->promoted_version != UINT32_MAX) {
                if (cpeed_device->target_version >= extension->promoted_version) {
                    extension->load_method = CpdVulkanExtensionLoadMethod_FromCore;
                    break;
                }

                if (cpeed_device->api_version >= extension->promoted_version) {
                    cpeed_device->target_version = extension->promoted_version;
                    extension->load_method = CpdVulkanExtensionLoadMethod_FromCore;
                    break;
                }
            }

            extension->load_method = CpdVulkanExtensionLoadMethod_FromExtension;
            loaded_from_extension++;
            break;
        }
    }

    free(properties);

    CpdVulkanExtensions* platform_extensions = (CpdVulkanExtensions*)malloc(sizeof(CpdVulkanExtensions));
    if (platform_extensions == 0) {
        return 0;
    }

    if (loaded_from_extension == 0) {
        platform_extensions->count = 0;
        return platform_extensions;
    }

    const char** all_extensions = (const char**)malloc((size_t)loaded_from_extension * sizeof(const char*));
    if (all_extensions == 0) {
        free(platform_extensions);
        return 0;
    }

    uint32_t copied_extensions = 0;
    for (uint32_t i = 0; i < cpeed_device->extension_count; i++) {
        if (cpeed_device->extensions[i].load_method == CpdVulkanExtensionLoadMethod_NotLoaded) {
            log_error("Missing device extension: %s\n", cpeed_device->extensions[i].name);
            continue;
        }

        if (cpeed_device->extensions[i].load_method != CpdVulkanExtensionLoadMethod_FromExtension) {
            continue;
        }

        all_extensions[copied_extensions++] = cpeed_device->extensions[i].name;
        log_debug("Enabling device extension: %s\n", cpeed_device->extensions[i].name);
    }

    platform_extensions->extensions = all_extensions;
    platform_extensions->count = copied_extensions;

    return platform_extensions;
}

static void free_device_extensions(CpdVulkanExtensions* extensions) {
    if (extensions->count > 0) {
        free(extensions->extensions);
    }

    free(extensions);
}

static VkResult create_logical_device(CpdDevice* cpeed_device, CpdDeviceInitParams* params) {
    uint32_t queue_create_info_count = 0;
    VkDeviceQueueCreateInfo* queue_create_info = (VkDeviceQueueCreateInfo*)0;
    
    VkResult result = allocate_queue_create_infos(
        params->graphics_family, params->compute_family, params->transfer_family,
        params->transfer_count + params->transfer_offset,
        &queue_create_info_count, &queue_create_info);

    if (result != VK_SUCCESS) {
        return result;
    }

    CpdVulkanExtensions* extensions = alloc_device_extensions(cpeed_device, params);

    CpdVulkanExtensionLoadMethod load_method = get_extension_load_method(cpeed_device, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    VulkanStructureChain* next;

    if (load_method == CpdVulkanExtensionLoadMethod_FromCore) {
        VkPhysicalDeviceVulkan13Features* features = (VkPhysicalDeviceVulkan13Features*)malloc(sizeof(VkPhysicalDeviceVulkan13Features));
        if (features == 0) {
            destroy_queue_create_infos(queue_create_info, queue_create_info_count);
            free_device_extensions(extensions);

            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        memset(features, 0, sizeof(VkPhysicalDeviceVulkan13Features));

        features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features->pNext = 0;
        features->dynamicRendering = VK_TRUE;
        features->synchronization2 = VK_TRUE;

        next = (VulkanStructureChain*)features;
    }
    else {
        VkPhysicalDeviceDynamicRenderingFeatures* dynamic_rendering_features = (VkPhysicalDeviceDynamicRenderingFeatures*)malloc(sizeof(VkPhysicalDeviceDynamicRenderingFeatures));
        VkPhysicalDeviceSynchronization2Features* synchronization2_features = (VkPhysicalDeviceSynchronization2Features*)malloc(sizeof(VkPhysicalDeviceSynchronization2Features));
        
        if (dynamic_rendering_features == 0 || synchronization2_features == 0) {
            free(dynamic_rendering_features);
            free(synchronization2_features);

            destroy_queue_create_infos(queue_create_info, queue_create_info_count);
            free_device_extensions(extensions);

            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        memset(dynamic_rendering_features, 0, sizeof(VkPhysicalDeviceDynamicRenderingFeatures));
        memset(synchronization2_features, 0, sizeof(VkPhysicalDeviceSynchronization2Features));

        dynamic_rendering_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_features->pNext = synchronization2_features;
        dynamic_rendering_features->dynamicRendering = VK_TRUE;

        synchronization2_features->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        synchronization2_features->pNext = 0;
        synchronization2_features->synchronization2 = VK_TRUE;

        next = (VulkanStructureChain*)dynamic_rendering_features;
    }

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = next,
        .flags = 0,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = 0,
        .enabledExtensionCount = extensions->count,
        .ppEnabledExtensionNames = extensions->count > 0 ? extensions->extensions : 0,
        .pEnabledFeatures = 0
    };

    result = vkCreateDevice(params->physical_device, &create_info, VK_NULL_HANDLE, &cpeed_device->handle);

    while (next != 0) {
        VulkanStructureChain* current_chain = next;
        next = next->pNext;

        free(current_chain);
    }

    destroy_queue_create_infos(queue_create_info, queue_create_info_count);
    free_device_extensions(extensions);
    return result;
}

VkResult DEVICE_initialize(CpdDevice* cpeed_device, CpdDeviceInitParams* params) {
    VkPhysicalDeviceProperties2 physical_device_properties = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = 0
    };
    get_physical_device_properties(params->physical_device, &physical_device_properties);

    cpeed_device->physical_handle = params->physical_device;

    cpeed_device->api_version = physical_device_properties.properties.apiVersion;
    cpeed_device->target_version = min_u32(VK_API_VERSION_1_3, cpeed_device->api_version);

    cpeed_device->extensions = params->device_extensions;
    cpeed_device->extension_count = params->device_extension_count;

    VkResult result = create_logical_device(cpeed_device, params);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->graphics_family.index = params->graphics_family;
    cpeed_device->compute_family.index = params->compute_family;
    cpeed_device->transfer_family.index = params->transfer_family;
    cpeed_device->transfer_family.queue_count = params->transfer_count;

    init_device_functions(cpeed_device);

    cpeed_device->vkGetDeviceQueue(cpeed_device->handle, cpeed_device->graphics_family.index, 0, &cpeed_device->graphics_family.queue);
    result = init_device_pool(cpeed_device, &cpeed_device->graphics_family);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->vkGetDeviceQueue(cpeed_device->handle, cpeed_device->compute_family.index, 0, &cpeed_device->compute_family.queue);
    result = init_device_pool(cpeed_device, &cpeed_device->compute_family);
    if (result != VK_SUCCESS) {
        return result;
    }

    cpeed_device->transfer_family.queues = (CpdTransferQueue*)malloc(cpeed_device->transfer_family.queue_count * sizeof(CpdTransferQueue));
    if (cpeed_device->transfer_family.queues == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    log_debug("Initializing %s with %d transfer queue(s) and name \"%s\"\n",
        string_VkPhysicalDeviceType(physical_device_properties.properties.deviceType),
        cpeed_device->transfer_family.queue_count,
        physical_device_properties.properties.deviceName);

    for (uint32_t i = 0; i < cpeed_device->transfer_family.queue_count; i++) {
        cpeed_device->transfer_family.queues[i].bytes_queued = 0;
        cpeed_device->vkGetDeviceQueue(cpeed_device->handle, cpeed_device->transfer_family.index, i + params->transfer_offset, &cpeed_device->transfer_family.queues[i].handle);
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
