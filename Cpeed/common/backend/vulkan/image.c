#include "image.h"

CpdImage* IMAGE_create(CpdDevice* cpeed_device, VkImageCreateInfo* create_info) {
    create_info->mipLevels = min_u32(log2_u32(create_info->extent.width), log2_u32(create_info->extent.height));
    create_info->arrayLayers = 1;
    create_info->samples = VK_SAMPLE_COUNT_1_BIT;

    // TODO: Create image
}

void IMAGE_destroy(CpdImage* cpeed_image, CpdDevice* cpeed_device) {
    if (cpeed_image->memory.handle != VK_NULL_HANDLE) {
        // TODO: Signal to free memory region
    }

    if (cpeed_image->view != VK_NULL_HANDLE) {
        cpeed_device->vkDestroyImageView(cpeed_device->handle, cpeed_image->view, VK_NULL_HANDLE);
    }

    cpeed_device->vkDestroyImage(cpeed_device->handle, cpeed_image->handle, VK_NULL_HANDLE);
}
