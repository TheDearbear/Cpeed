#include <malloc.h>

#include <Cpeed/platform/logging.h>

#include "surface.h"
#include "swapchain.h"

VkResult SURFACE_initialize(CpdSurface* cpeed_surface, CpdDevice* cpeed_device, VkSurfaceKHR surface) {
    uint32_t format_count;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(cpeed_device->physical_handle, surface, &format_count, VK_NULL_HANDLE);
    if (result != VK_SUCCESS) {
        return result;
    }

    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(format_count * sizeof(VkSurfaceFormatKHR));
    if (formats == 0) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(cpeed_device->physical_handle, surface, &format_count, formats);
    if (result != VK_SUCCESS) {
        free(formats);
        return result;
    }

    cpeed_surface->format = formats[format_count - 1];
    free(formats);

    VkBool32 supported = VK_FALSE;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(
        cpeed_device->physical_handle,
        cpeed_device->graphics_family.index,
        surface,
        &supported);
    if (result != VK_SUCCESS) {
        return result;
    }

    if (!supported) {
        log_error("Presentation is not supported by selected queue family\n");
        return VK_ERROR_UNKNOWN;
    }

    cpeed_surface->handle = surface;
    return result;
}
