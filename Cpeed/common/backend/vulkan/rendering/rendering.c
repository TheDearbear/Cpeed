#include <stdio.h>

#include "rendering.h"
#include "../shortcuts.h"

VkCommandBuffer buffer = VK_NULL_HANDLE;
VkFence render_fence = VK_NULL_HANDLE;

float brightness_red = 1.0f;
float brightness_green = 1.0f;
float brightness_blue = 1.0f;
float brightness_step = 0.05f;

VkResult RENDERING_initialize(CpdRenderer* cpeed_renderer) {
    VkResult result = create_fence(&cpeed_renderer->render_device, &render_fence);
    if (result != VK_SUCCESS) {
        printf("Unable to create render fence. Result code: %s\n", string_VkResult(result));
        return result;
    }

    result = create_primary_command_buffer(&cpeed_renderer->render_device, cpeed_renderer->render_device.graphics_family.pool, &buffer);
    if (result != VK_SUCCESS) {
        printf("Unable to create command buffer. Result code: %s\n", string_VkResult(result));
        return result;
    }

    return VK_SUCCESS;
}

VkResult RENDERING_resize(CpdRenderer* cpeed_renderer, CpdSize new_size) {
    return VK_SUCCESS;
}

void RENDERING_input(CpdRenderer* cpeed_renderer, CpdWindow window, const CpdInputEvent* events, uint32_t event_count) {
    for (uint32_t i = 0; i < event_count; i++) {
        const CpdInputEvent* event = &events[i];

        if (event->type == CpdInputEventType_CharInput) {
            uint64_t text = event->data.char_input.character;
            printf("Char input: %s (%d bytes)\n", (char*)&text, event->data.char_input.length);
        }
        else if (event->type == CpdInputEventType_ButtonPress) {
            if (event->data.button_press.pressed) {
                if (event->data.button_press.key_code == CpdKeyCode_Numpad7 && brightness_red < 1.0f) {
                    brightness_red += brightness_step;
                    printf("New red brightness: %.2f\n", brightness_red);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad4 && brightness_red > 0.0f) {
                    brightness_red -= brightness_step;
                    printf("New red brightness: %.2f\n", brightness_red);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad8 && brightness_green < 1.0f) {
                    brightness_green += brightness_step;
                    printf("New green brightness: %.2f\n", brightness_green);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad5 && brightness_green > 0.0f) {
                    brightness_green -= brightness_step;
                    printf("New green brightness: %.2f\n", brightness_green);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad9 && brightness_blue < 1.0f) {
                    brightness_blue += brightness_step;
                    printf("New blue brightness: %.2f\n", brightness_blue);
                }
                else if (event->data.button_press.key_code == CpdKeyCode_Numpad6 && brightness_blue > 0.0f) {
                    brightness_blue -= brightness_step;
                    printf("New blue brightness: %.2f\n", brightness_blue);
                }
            }
            else if (event->data.button_press.key_code == CpdKeyCode_Escape) {
                close_window(window);
            }
        }
        else if (event->type == CpdInputEventType_Clipboard) {
            if (event->data.clipboard.action_type == CpdClipboardActionType_Paste) {
                printf("Paste data\n");
            }
            else if (event->data.clipboard.action_type == CpdClipboardActionType_Copy) {
                printf("Copy data\n");
            }
            else if (event->data.clipboard.action_type == CpdClipboardActionType_Cut) {
                printf("Cut data\n");
            }
        }
    }
}

static void begin_rendering(CpdRenderer* cpeed_renderer, VkCommandBuffer buffer) {
    CpdImage* image = &cpeed_renderer->swapchain.images[cpeed_renderer->swapchain.current_image].image;

    VkRenderingAttachmentInfo attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image->view,
        .imageLayout = image->layout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue.color.float32[0] = brightness_red,
        .clearValue.color.float32[1] = brightness_green,
        .clearValue.color.float32[2] = brightness_blue,
        .clearValue.color.float32[3] = 1.0f
    };

    VkRenderingInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea.offset = (VkOffset2D){ 0, 0 },
        .renderArea.extent.width = cpeed_renderer->swapchain.size.width,
        .renderArea.extent.height = cpeed_renderer->swapchain.size.height,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment
    };

    cpeed_renderer->render_device.vkCmdBeginRendering(buffer, &info);
}

VkResult RENDERING_frame(CpdRenderer* cpeed_renderer) {
    VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = VK_NULL_HANDLE
    };

    VkResult result = cpeed_renderer->render_device.vkBeginCommandBuffer(buffer, &begin_info);
    if (result != VK_SUCCESS) {
        printf("Unable to begin command buffer. Result code: %s\n", string_VkResult(result));
        return result;
    }

    if (cpeed_renderer->swapchain.wait_for_acquire) {
        result = cpeed_renderer->render_device.vkWaitForFences(cpeed_renderer->render_device.handle, 1, &cpeed_renderer->swapchain_image_fence, VK_FALSE, UINT64_MAX);
        if (result != VK_SUCCESS) {
            printf("Acquiring image of swapchain failed. Result code: %s\n", string_VkResult(result));
            return result;
        }
    }

    SWAPCHAIN_set_layout(&cpeed_renderer->swapchain, &cpeed_renderer->render_device, buffer,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    begin_rendering(cpeed_renderer, buffer);

    cpeed_renderer->render_device.vkCmdEndRendering(buffer);

    SWAPCHAIN_set_layout(&cpeed_renderer->swapchain, &cpeed_renderer->render_device, buffer,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    result = cpeed_renderer->render_device.vkEndCommandBuffer(buffer);
    if (result != VK_SUCCESS) {
        printf("Unable to end command buffer. Result code: %s\n", string_VkResult(result));
        return result;
    }

    VkCommandBufferSubmitInfo buffer_submit_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = 0,
        .commandBuffer = buffer,
        .deviceMask = 0
    };

    VkSemaphore current_semaphore = cpeed_renderer->swapchain.images[cpeed_renderer->swapchain.current_image].semaphore;

    VkSemaphoreSubmitInfoKHR signal_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = 0,
        .semaphore = current_semaphore,
        .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .deviceIndex = 0
    };

    VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = 0,
        .flags = 0,
        .waitSemaphoreInfoCount = 0,
        .pWaitSemaphoreInfos = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &buffer_submit_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signal_info
    };

    result = cpeed_renderer->render_device.vkResetFences(cpeed_renderer->render_device.handle, 1, &render_fence);
    if (result != VK_SUCCESS) {
        printf("Unable to reset render fence. Result code: %s\n", string_VkResult(result));
    }

    result = cpeed_renderer->render_device.vkQueueSubmit2(cpeed_renderer->render_device.graphics_family.queue, 1, &submit_info, render_fence);
    if (result != VK_SUCCESS) {
        printf("Unable to queue work. Result code: %s\n", string_VkResult(result));
    }

    return cpeed_renderer->render_device.vkWaitForFences(cpeed_renderer->render_device.handle, 1, &render_fence, VK_TRUE, UINT64_MAX);
}

void RENDERING_shutdown(CpdRenderer* cpeed_renderer) {
    if (buffer != VK_NULL_HANDLE) {
        cpeed_renderer->render_device.vkFreeCommandBuffers(cpeed_renderer->render_device.handle, cpeed_renderer->render_device.graphics_family.pool, 1, &buffer);
        buffer = VK_NULL_HANDLE;
    }

    if (render_fence != VK_NULL_HANDLE) {
        cpeed_renderer->render_device.vkDestroyFence(cpeed_renderer->render_device.handle, render_fence, VK_NULL_HANDLE);
        render_fence = VK_NULL_HANDLE;
    }
}
