#include "vulkan.h"

#define DEFINE_INSTANCE_FUNCTION(name) PFN_ ## name name = VK_NULL_HANDLE;

#define FILL_VULKAN_EXTENSION_INFO(extensions, field_name, extension_name, promoted) \
    (extensions)->field_name.name = extension_name; \
    (extensions)->field_name.promoted_version = promoted; \
    (extensions)->field_name.load_method = CpdVulkanExtensionLoadMethod_NotLoaded;

// == Global

DEFINE_INSTANCE_FUNCTION(vkGetInstanceProcAddr);
DEFINE_INSTANCE_FUNCTION(vkEnumerateInstanceVersion);

// == Instance

DEFINE_INSTANCE_FUNCTION(vkCreateInstance);
DEFINE_INSTANCE_FUNCTION(vkDestroyInstance);
DEFINE_INSTANCE_FUNCTION(vkEnumerateInstanceExtensionProperties);

// == Physical Device

DEFINE_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties2);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties2);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
DEFINE_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

// == Logical Device

DEFINE_INSTANCE_FUNCTION(vkCreateDevice);
DEFINE_INSTANCE_FUNCTION(vkGetDeviceProcAddr);
DEFINE_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties);

// == Surface

DEFINE_INSTANCE_FUNCTION(vkDestroySurfaceKHR);

// == Functions

void initialize_vulkan_instance_extensions(CpdInstanceVulkanExtensions* extensions) {
    FILL_VULKAN_EXTENSION_INFO(extensions, get_physical_device_properties2,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_API_VERSION_1_1);
}

void initialize_vulkan_render_device_extensions(CpdRenderDeviceVulkanExtensions* extensions) {
    FILL_VULKAN_EXTENSION_INFO(extensions, create_renderpass2,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_API_VERSION_1_2);
    
    FILL_VULKAN_EXTENSION_INFO(extensions, depth_stencil_resolve,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_API_VERSION_1_2);
    
    FILL_VULKAN_EXTENSION_INFO(extensions, dynamic_rendering,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_API_VERSION_1_3);
    
    FILL_VULKAN_EXTENSION_INFO(extensions, maintenance2,
        VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
        VK_API_VERSION_1_1);
    
    FILL_VULKAN_EXTENSION_INFO(extensions, multiview,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_API_VERSION_1_1);
    
    FILL_VULKAN_EXTENSION_INFO(extensions, synchronization2,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_API_VERSION_1_3);

    FILL_VULKAN_EXTENSION_INFO(extensions, swapchain,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        UINT32_MAX);
}

void get_physical_device_properties(VkPhysicalDevice physical_device, VkPhysicalDeviceProperties2* properties);
