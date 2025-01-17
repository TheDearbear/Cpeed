#pragma once

#define VK_NO_PROTOTYPES

#include <stdbool.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "platform.h"

PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
