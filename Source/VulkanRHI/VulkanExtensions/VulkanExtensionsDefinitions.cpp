#include <vulkan/vulkan.h>

#include "VulkanExtensionsDefinitions.h"

import VulkanExtensions;

// VK_EXT_debug_utils
PFN_vkCreateDebugUtilsMessengerEXT  pfn_vkCreateDebugUtilsMessengerEXT = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
PFN_vkSetDebugUtilsObjectNameEXT    pfn_vkSetDebugUtilsObjectNameEXT = nullptr;

// VK_EXT_debug_utils
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger) {
	return pfn_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance                   instance,
														   VkDebugUtilsMessengerEXT     messenger,
														   VkAllocationCallbacks const* pAllocator) {
	return pfn_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice                             device,
															const VkDebugUtilsObjectNameInfoEXT* pNameInfo) {
	return pfn_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}