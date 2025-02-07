#include <malloc.h>

#include "swapchain.h"

static VkResult clamp_swapchain(CpdDevice* device, CpdSurface* surface, CpdSize* size, uint32_t* image_count) {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_handle, surface->handle, &surface_capabilities);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Clamp width

    unsigned short width = (unsigned short)max_u32(surface_capabilities.minImageExtent.width, size->width);
    size->width = surface_capabilities.maxImageExtent.width == 0xFFFFFFFF ?
        width :
        (unsigned short)min_u32(surface_capabilities.maxImageExtent.width, width);

    // Clamp height

    unsigned short height = (unsigned short)max_u32(surface_capabilities.minImageExtent.height, size->height);
    size->height = surface_capabilities.maxImageExtent.height == 0xFFFFFFFF ?
        height :
        (unsigned short)min_u32(surface_capabilities.maxImageExtent.height, height);

    // Clamp image count

    const uint32_t base_image_count = 3;
    uint32_t min_image_count = max_u32(surface_capabilities.minImageCount, base_image_count);

    *image_count = surface_capabilities.maxImageCount == 0 ?
        min_image_count :
        min_u32(min_image_count, surface_capabilities.maxImageCount);

    return result;
}

void SWAPCHAIN_destroy(CpdSwapchain* swapchain, CpdDevice* cpeed_device) {
    for (uint32_t i = 0; i < swapchain->image_count; i++) {
        cpeed_device->vkDestroyImageView(cpeed_device->handle, swapchain->images[i].view, VK_NULL_HANDLE);
    }

    cpeed_device->vkDestroySwapchainKHR(cpeed_device->handle, swapchain->handle, VK_NULL_HANDLE);
    free(swapchain->images);

    swapchain->handle = VK_NULL_HANDLE;
    swapchain->images = VK_NULL_HANDLE;
    swapchain->image_count = 0;
    swapchain->current_image = 0;
}

VkResult SWAPCHAIN_create(CpdSwapchain* swapchain, CpdDevice* cpeed_device, CpdSurface* surface, CpdSize* size) {
    uint32_t image_count;
    VkResult result = clamp_swapchain(cpeed_device, surface, size, &image_count);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
        .surface = surface->handle,
        .minImageCount = image_count,
        .imageFormat = surface->format.format,
        .imageColorSpace = surface->format.colorSpace,
        .imageExtent.width = size->width,
        .imageExtent.height = size->height,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = cpeed_device->graphics_family.index,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    result = cpeed_device->vkCreateSwapchainKHR(cpeed_device->handle, &create_info, VK_NULL_HANDLE, &swapchain->handle);
    if (result == VK_SUCCESS) {
        swapchain->current_image = 0;

        result = cpeed_device->vkGetSwapchainImagesKHR(
            cpeed_device->handle,
            swapchain->handle,
            &swapchain->image_count,
            VK_NULL_HANDLE);
        if (result != VK_SUCCESS) {
            SWAPCHAIN_destroy(swapchain, cpeed_device);
            return result;
        }

        VkImage* images = (VkImage*)malloc(swapchain->image_count * sizeof(VkImage));
        if (images == 0) {
            SWAPCHAIN_destroy(swapchain, cpeed_device);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        result = cpeed_device->vkGetSwapchainImagesKHR(
            cpeed_device->handle,
            swapchain->handle,
            &swapchain->image_count,
            images);
        if (result != VK_SUCCESS) {
            SWAPCHAIN_destroy(swapchain, cpeed_device);
            free(images);
            return result;
        }

        swapchain->images = (CpdImage*)malloc(swapchain->image_count * sizeof(CpdImage));
        if (swapchain->images == 0) {
            SWAPCHAIN_destroy(swapchain, cpeed_device);
            free(images);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        for (uint32_t i = 0; i < swapchain->image_count; i++) {
            CpdImage* image = &swapchain->images[i];

            image->handle = images[i];
            image->stage = VK_PIPELINE_STAGE_2_NONE_KHR;
            image->access = VK_ACCESS_2_NONE_KHR;
            image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
            image->format = surface->format.format;
            image->queue_family_index = cpeed_device->graphics_family.index;
            image->size.width = size->width;
            image->size.height = size->height;

            VkImageSubresourceRange range = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS
            };

            VkImageViewCreateInfo view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = VK_NULL_HANDLE,
                .flags = 0,
                .image = image->handle,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = image->format,
                .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                .subresourceRange = range
            };

            result = cpeed_device->vkCreateImageView(cpeed_device->handle, &view_create_info, VK_NULL_HANDLE, &image->view);
            if (result != VK_SUCCESS) {
                // Prevent destroying of uncreated views
                swapchain->image_count = i;

                SWAPCHAIN_destroy(swapchain, cpeed_device);
                free(images);
                return result;
            }
        }

        free(images);
    }

    return result;
}

VkResult SWAPCHAIN_resize(CpdSwapchain* swapchain, CpdDevice* cpeed_device, CpdSurface* surface, CpdSize* size) {
    if (swapchain->handle != VK_NULL_HANDLE) {
        SWAPCHAIN_destroy(swapchain, cpeed_device);
    }

    return SWAPCHAIN_create(swapchain, cpeed_device, surface, size);
}

void SWAPCHAIN_set_layout(CpdSwapchain* swapchain, CpdDevice* cpeed_device, VkCommandBuffer buffer,
    VkPipelineStageFlags2KHR stage, VkAccessFlags2KHR access, VkImageLayout layout
) {
    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = 0,
        .layerCount = VK_REMAINING_ARRAY_LAYERS
    };

    CpdImage* image = &swapchain->images[swapchain->current_image];

    VkImageMemoryBarrier2KHR image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
        .pNext = VK_NULL_HANDLE,
        .srcStageMask = image->stage,
        .srcAccessMask = image->access,
        .dstStageMask = stage,
        .dstAccessMask = access,
        .oldLayout = image->layout,
        .newLayout = layout,
        .srcQueueFamilyIndex = image->queue_family_index,
        .dstQueueFamilyIndex = image->queue_family_index,
        .image = image->handle,
        .subresourceRange = range
    };

    VkDependencyInfoKHR dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
        .pNext = VK_NULL_HANDLE,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = VK_NULL_HANDLE,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = VK_NULL_HANDLE,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier
    };

    image->stage = stage;
    image->access = access;
    image->layout = layout;

    cpeed_device->vkCmdPipelineBarrier2KHR(buffer, &dependency_info);
}
