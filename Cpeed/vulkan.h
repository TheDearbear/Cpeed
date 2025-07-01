#pragma once

#define VK_NO_PROTOTYPES

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

typedef enum CpdVulkanExtensionLoadMethod {
    CpdVulkanExtensionLoadMethod_NotLoaded = 0,
    CpdVulkanExtensionLoadMethod_FromExtension,
    CpdVulkanExtensionLoadMethod_FromCore
} CpdVulkanExtensionLoadMethod;

typedef struct CpdVulkanExtension {
    const char* name;
    uint32_t promoted_version;
    CpdVulkanExtensionLoadMethod load_method;
} CpdVulkanExtension;

#define GET_EXTENSIONS_COUNT(struct_name) ((uint32_t)(sizeof(struct_name) / sizeof(CpdVulkanExtension)))

// All used Vulkan instance extensions
typedef struct CpdInstanceVulkanExtensions {
    CpdVulkanExtension get_physical_device_properties2;
} CpdInstanceVulkanExtensions;

// All used Vulkan device extensions
typedef struct CpdRenderDeviceVulkanExtensions {
    CpdVulkanExtension create_renderpass2;
    CpdVulkanExtension depth_stencil_resolve;
    CpdVulkanExtension dynamic_rendering;
    CpdVulkanExtension maintenance2;
    CpdVulkanExtension multiview;
    CpdVulkanExtension synchronization2;
    CpdVulkanExtension swapchain;
} CpdRenderDeviceVulkanExtensions;

struct VulkanStructureChain;

typedef struct VulkanStructureChain {
    VkStructureType sType;
    struct VulkanStructureChain* pNext;
} VulkanStructureChain;

// == Global

extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;

// == Instance

extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;

// == Physical Device

extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;

// == Logical Device

extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

// == Surface

extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;

// == Functions

void initialize_vulkan_instance_extensions(CpdInstanceVulkanExtensions* extensions);
void initialize_vulkan_render_device_extensions(CpdRenderDeviceVulkanExtensions* extensions);

inline void get_physical_device_properties(VkPhysicalDevice physical_device, VkPhysicalDeviceProperties2* properties) {
    if (vkGetPhysicalDeviceProperties2 != VK_NULL_HANDLE) {
        vkGetPhysicalDeviceProperties2(physical_device, properties);
    }
    else {
        vkGetPhysicalDeviceProperties(physical_device, &properties->properties);
    }
}
