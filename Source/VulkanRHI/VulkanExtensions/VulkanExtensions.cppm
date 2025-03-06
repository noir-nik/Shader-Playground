export module VulkanExtensions;
import vulkan_hpp;

export {
	void LoadInstanceDebugUtilsFunctionsEXT(vk::Instance instance);
	void LoadDeviceDebugUtilsFunctionsEXT(vk::Device device);
}
