#pragma once

#define VK_NO_PROTOTYPES

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

// == Global

extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

// == Instance

extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;

// == Physical Device

extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;

// == Logical Device

extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

// == Surface

extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
