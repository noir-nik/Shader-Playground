module;

#include <vulkan/vulkan.h>

#include "VulkanExtensionsDefinitions.h"

module VulkanExtensions;

import vulkan_hpp;

void LoadInstanceDebugUtilsFunctionsEXT(vk::Instance instance) {
	pfn_vkCreateDebugUtilsMessengerEXT =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	pfn_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");
}

void LoadDeviceDebugUtilsFunctionsEXT(vk::Device device) {
	pfn_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
		vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectName"));
}
