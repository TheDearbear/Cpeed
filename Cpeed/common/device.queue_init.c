#include "device.queue_init.h"

void destroy_queue_create_infos(VkDeviceQueueCreateInfo* infos, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        #pragma warning(push)
        #pragma warning(disable: 6001)
        free((void*)infos[i].pQueuePriorities);
        #pragma warning(pop)
    }

    free(infos);
}

bool allocate_queue_create_infos(
    uint32_t graphics, uint32_t compute, uint32_t transfer,
    uint32_t transfer_count, uint32_t* count, VkDeviceQueueCreateInfo** infos
) {
    VkDeviceQueueCreateInfo local_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = 0,
    };

    if (graphics == compute && graphics == transfer) {
        float* priorities = (float*)malloc(transfer_count * sizeof(float));
        if (priorities == 0) {
            return false;
        }

        for (uint32_t i = 0; i < transfer_count; i++) {
            priorities[i] = 1.0f;
        }

        VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*)malloc(1 * sizeof(VkDeviceQueueCreateInfo));
        if (info == 0) {
            free(priorities);
            return false;
        }

        local_info.queueFamilyIndex = graphics;
        local_info.queueCount = transfer_count;
        local_info.pQueuePriorities = priorities;
        info[0] = local_info;

        *count = 1;
        *infos = info;

        return true;
    }
    else if (graphics == compute || graphics == transfer || compute == transfer) {
        uint32_t first_count = transfer_count;
        uint32_t second_count = graphics == compute ? 2 : 1;

        float* first_priorities = (float*)malloc(first_count * sizeof(float));
        if (first_priorities == 0) {
            return false;
        }

        for (uint32_t i = 0; i < first_count; i++) {
            first_priorities[i] = 1.0f;
        }

        float* second_priorities = (float*)malloc(second_count * sizeof(float));
        if (second_priorities == 0) {
            free(first_priorities);
            return false;
        }

        for (uint32_t i = 0; i < second_count; i++) {
            second_priorities[i] = 1.0f;
        }

        VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*)malloc(2 * sizeof(VkDeviceQueueCreateInfo));
        if (info == 0) {
            free(first_priorities);
            free(second_priorities);
            return false;
        }

        local_info.queueFamilyIndex = transfer;
        local_info.queueCount = first_count;
        local_info.pQueuePriorities = first_priorities;
        info[0] = local_info;

        local_info.queueFamilyIndex = transfer == compute ? graphics : compute;
        local_info.queueCount = second_count;
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

    float* transfer_priorities = (float*)malloc(transfer_count * sizeof(float));
    if (transfer_priorities == 0) {
        free(graphics_priorities);
        free(compute_priorities);
        return false;
    }

    graphics_priorities[0] = 1.0f;
    compute_priorities[0] = 1.0f;

    for (uint32_t i = 0; i < transfer_count; i++) {
        transfer_priorities[i] = 1.0f;
    }

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
    local_info.queueCount = transfer_count;
    local_info.pQueuePriorities = transfer_priorities;
    info[2] = local_info;

    *count = 3;
    *infos = info;

    return true;
}
