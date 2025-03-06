module;
#include "Log/LogMacros.hpp"
#include "Misc/CheckVulkanResult.hpp"
#include "Misc/VertexShaderCode.h"
#include "Shaders/PushConstants.h"
module Application;
import std;
import vulkan_hpp;
import VulkanRHI;
import MessageCallbacks;
import Window;
import WindowManager;
import Utils;
import VulkanExtensions;
import Constants;
import ShaderCompiler;
import Log;

using u32 = std::uint32_t;

struct PhysicalDevice : public VulkanRHI::PhysicalDevice {
	PhysicalDevice() {}
	bool IsSuitable(vk::SurfaceKHR const& surface, std::span<char const* const> const extensions) {
		bool const bSupportsExtensions = SupportsExtensions(extensions);
		bool const bSupportsQueues     = SupportsQueue({.flags = vk::QueueFlagBits::eGraphics, .surface = surface});
		if (bSupportsExtensions && bSupportsQueues) {
			return true;
		}
		return false;
	}

	u32 graphics_queue_family_index = std::numeric_limits<u32>::max();
};

struct MainAppImpl {
	static constexpr u32 kApiVersion = vk::ApiVersion13;

	static constexpr char const* kEnabledLayers[]           = {"VK_LAYER_KHRONOS_validation"};
	static constexpr char const* kEnabledDeviceExtensions[] = {
		vk::KHRSwapchainExtensionName,
	};

	~MainAppImpl();
	void Run(int argc, char const* const* argv);
	auto ParseArgs(int argc, char const* const* argv) -> char const*;
	void Init();
	void MainLoop();
	void Destroy();
	void CreateInstance();
	void SelectPhysicalDevice();
	void GetPhysicalDeviceInfo();
	void CreateDevice();
	// void CreateVmaAllocator();

	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateDescriptorSet();

	void CreateSwapchain();

	void CreatePipelineLayout();
	void CreateVertexShaderModule();

	// [[nodiscard]] auto CreatePipeline(std::string_view const fragment_shader_code) -> vk::Result;
	[[nodiscard]] auto CreatePipeline(std::span<std::byte const> fragment_shader_code) -> vk::Result;

	void RecordCommands();
	void DrawWindow();
	void RecreateSwapchain(int width, int height);

	auto GetAllocator() const -> vk::AllocationCallbacks const* { return allocator; }
	auto GetPipelineCache() const -> vk::PipelineCache { return pipeline_cache; }

	ShaderCompiler shader_compiler;

	bool bVerbose           = false;
	bool bValidationEnabled = true;

	std::string_view fragment_shader_path;

	Window window{};
	struct {
		float x = 300.0f;
		float y = 300.0f;
	} mouse;

	vk::Instance                   instance{};
	vk::AllocationCallbacks const* allocator{nullptr};
	// VmaAllocator                    vma_allocator{};
	vk::PipelineCache               pipeline_cache{nullptr};
	vk::DebugUtilsMessengerEXT      debug_messenger{};
	std::span<char const* const>    enabled_layers{};
	std::vector<vk::PhysicalDevice> vulkan_physical_devices{};
	PhysicalDevice                  physical_device{};
	vk::Device                      device{};
	VulkanRHI::Swapchain            swapchain{};
	bool                            swapchain_dirty = false;
	vk::SurfaceKHR                  surface{};

	vk::Queue queue{};
	u32       queue_family_index = ~0u;

	// vk::DescriptorSetLayout descriptor_set_layout{};
	// vk::DescriptorPool      descriptor_pool{};
	// vk::DescriptorSet       descriptor_set{};

	vk::PipelineLayout pipeline_layout{};
	vk::Pipeline       pipeline{};
	vk::ShaderModule   vertex_shader_module;
	vk::ShaderModule   fragment_shader_module;
};

static MainAppImpl* gAppData = nullptr;

static void FramebufferSizeCallback(Window* window, int width, int height) {
	gAppData->swapchain_dirty = true;
	if (width <= 0 || height <= 0) return;
	gAppData->RecreateSwapchain(width, height);
}

static void WindowRefreshCallback(Window* window) {
	int x, y, width, height;
	window->GetRect(x, y, width, height);
	if (width <= 0 || height <= 0) return;
	gAppData->DrawWindow();
}

static void CursorPosCallback(Window* window, double xpos, double ypos) {
	gAppData->mouse.x = static_cast<float>(xpos);
	gAppData->mouse.y = static_cast<float>(ypos);
}

static void KeyCallback(Window* window, int key, int scancode, int action, int mods) {
	switch (key) {
	case 256:
		window->SetShouldClose(true);
		break;
	}
}

void MainAppImpl::Init() {
	WindowManager::SetErrorCallback(WindowErrorCallback);
	WindowManager::Init();
	constexpr u32 title_buffer_size = 256 + Constants::kApplicationTitle.size() + 3;
	char          title_buffer[title_buffer_size];

	std::snprintf(title_buffer, sizeof(title_buffer) - 1, "%ls - %s",
				  std::filesystem::path(fragment_shader_path).filename().c_str(),
				  Constants::kApplicationTitle.data());

	window.Init({.x = 30, .y = 30, .width = 800, .height = 600, .title = title_buffer});
	window.GetWindowCallbacks().framebufferSizeCallback = FramebufferSizeCallback;
	window.GetWindowCallbacks().windowRefreshCallback   = WindowRefreshCallback;
	window.GetInputCallbacks().cursorPosCallback        = CursorPosCallback;
	window.GetInputCallbacks().keyCallback              = KeyCallback;
	CreateInstance();

	CHECK_RESULT(WindowManager::CreateWindowSurface(instance, reinterpret_cast<GLFWwindow*>(window.GetHandle()), GetAllocator(), &surface));

	SelectPhysicalDevice();
	GetPhysicalDeviceInfo();

	CreateDevice();
	// CreateVmaAllocator();

	// CreateDescriptorSetLayout();
	// CreateDescriptorPool();
	// CreateDescriptorSet();

	CreateSwapchain();

	shader_compiler.Init();

	CreatePipelineLayout();
	CreateVertexShaderModule();
	// std::optional<std::string_view> shader_code = shader_compiler.LoadShader(fragment_shader_path);

	std::optional<std::span<const std::byte>> shader_code = shader_compiler.LoadShader(fragment_shader_path);
	if (!shader_code.has_value()) {
		LOG_FATAL("Failed to read fragment shader file: %s", shader_compiler.GetErrorMessage().data());
	}
	if (shader_code.value().empty()) {
		LOG_FATAL("Fragment shader file %s is empty!", fragment_shader_path.data());
	}

	// auto       shader_code = Utils::ReadBinaryFile(std::string(fragment_shader_path) + ".spv");
	vk::Result result      = CreatePipeline(shader_code.value());
	CHECK_RESULT(result);
}

MainAppImpl::~MainAppImpl() { Destroy(); }

void MainAppImpl::Destroy() {

	if (device) {
		CHECK_RESULT(device.waitIdle());

		device.destroyPipeline(pipeline, GetAllocator());
		device.destroyShaderModule(vertex_shader_module, GetAllocator());
		device.destroyPipelineLayout(pipeline_layout, GetAllocator());

		// device.destroyDescriptorSetLayout(descriptor_set_layout, GetAllocator());
		// device.destroyDescriptorPool(descriptor_pool, GetAllocator());

		swapchain.Destroy();
		device.destroyPipelineCache(pipeline_cache, GetAllocator());

		// vmaDestroyAllocator(vma_allocator);

		device.destroy(GetAllocator());
		device = vk::Device{};
	}

	if (instance) {
		instance.destroySurfaceKHR(surface, GetAllocator());
		if (bValidationEnabled) {
			instance.destroyDebugUtilsMessengerEXT(debug_messenger, GetAllocator());
		}
		instance.destroy(GetAllocator());
		instance = vk::Instance{};
	}
	window.Destroy();
	WindowManager::Terminate();
}

void MainAppImpl::CreateInstance() {
	vk::ApplicationInfo applicationInfo{.apiVersion = kApiVersion};

	u32 glfw_extensions_count;

	char const** glfw_extensions = WindowManager::GetRequiredInstanceExtensions(&glfw_extensions_count);

	std::vector<char const*> enabledExtensions(glfw_extensions, glfw_extensions + glfw_extensions_count);
	if (bValidationEnabled) {
		enabled_layers = kEnabledLayers;
		enabledExtensions.push_back(vk::EXTDebugUtilsExtensionName);
	}

	vk::DebugUtilsMessengerCreateInfoEXT constexpr kDebugUtilsCreateInfo = {
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
						   //    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
						   //    vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
		.pfnUserCallback = DebugUtilsCallback,
		.pUserData       = nullptr,
	};

	vk::InstanceCreateInfo info{
		.pNext                   = bValidationEnabled ? &kDebugUtilsCreateInfo : nullptr,
		.pApplicationInfo        = &applicationInfo,
		.enabledLayerCount       = static_cast<u32>(std::size(enabled_layers)),
		.ppEnabledLayerNames     = enabled_layers.data(),
		.enabledExtensionCount   = static_cast<u32>(std::size(enabledExtensions)),
		.ppEnabledExtensionNames = enabledExtensions.data(),
	};
	CHECK_RESULT(vk::createInstance(&info, GetAllocator(), &instance));

	if (bValidationEnabled) {
		LoadInstanceDebugUtilsFunctionsEXT(instance);
		CHECK_RESULT(instance.createDebugUtilsMessengerEXT(&kDebugUtilsCreateInfo, allocator, &debug_messenger));
	}

	vk::Result result;
	std::tie(result, vulkan_physical_devices) = instance.enumeratePhysicalDevices();
	CHECK_RESULT(result);
}

void MainAppImpl::SelectPhysicalDevice() {
	for (vk::PhysicalDevice const& device : vulkan_physical_devices) {
		physical_device.Assign(device);
		CHECK_RESULT(physical_device.GetDetails());
		if (physical_device.IsSuitable(surface, kEnabledDeviceExtensions)) {
			if (bVerbose) {
			}
			return;
		}
	}

	std::printf("No suitable physical device found\n");
	// std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	std::getchar();
	std::exit(1);
}

void MainAppImpl::GetPhysicalDeviceInfo() {
}

void MainAppImpl::CreateDevice() {
	float const queue_priorities[] = {1.0f};

	auto [result, index] = physical_device.GetQueueFamilyIndex({.flags = vk::QueueFlagBits::eGraphics, .surface = surface});
	if (result != vk::Result::eSuccess || !index.has_value()) {
		std::printf("Failed to get graphics queue family index with surface support\n");
		CHECK_RESULT(result);
	}

	queue_family_index = index.value();

	vk::DeviceQueueCreateInfo queue_create_infos[] = {
		{
			.queueFamilyIndex = queue_family_index,
			.queueCount       = 1,
			.pQueuePriorities = queue_priorities,
		}};

	vk::StructureChain features{
		vk::PhysicalDeviceFeatures2{},
		vk::PhysicalDeviceVulkan11Features{.storageBuffer16BitAccess = vk::True},
		vk::PhysicalDeviceVulkan12Features{.shaderFloat16 = vk::True, .bufferDeviceAddress = vk::True},
		vk::PhysicalDeviceVulkan13Features{.synchronization2 = vk::True, .dynamicRendering = vk::True},
		vk::PhysicalDeviceCooperativeVectorFeaturesNV{.cooperativeVector = vk::True, .cooperativeVectorTraining = vk::True},
		vk::PhysicalDeviceShaderReplicatedCompositesFeaturesEXT{.shaderReplicatedComposites = vk::True},
	};

	vk::DeviceCreateInfo info{
		.pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount    = static_cast<u32>(std::size(queue_create_infos)),
		.pQueueCreateInfos       = queue_create_infos,
		.enabledLayerCount       = static_cast<u32>(std::size(enabled_layers)),
		.ppEnabledLayerNames     = enabled_layers.data(),
		.enabledExtensionCount   = static_cast<u32>(std::size(kEnabledDeviceExtensions)),
		.ppEnabledExtensionNames = kEnabledDeviceExtensions,
	};

	CHECK_RESULT(physical_device.createDevice(&info, GetAllocator(), &device));
	queue = device.getQueue(queue_create_infos[0].queueFamilyIndex, 0);
}

void MainAppImpl::CreateSwapchain() {
	int x, y, width, height;
	window.GetRect(x, y, width, height);
	VulkanRHI::SwapchainInfo info{
		.surface            = surface,
		.extent             = {.width = static_cast<u32>(width), .height = static_cast<u32>(height)},
		.queue_family_index = queue_family_index,
	};
	CHECK_RESULT(swapchain.Create(device, physical_device, info, GetAllocator()));
}

void MainAppImpl::CreatePipelineLayout() {
	vk::PushConstantRange push_constant_range{
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
		.offset     = 0,
		.size       = physical_device.GetMaxPushConstantsSize(),
	};

	vk::PipelineLayoutCreateInfo info{
		.setLayoutCount = 0,
		// .pSetLayouts            = &descriptor_set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges    = &push_constant_range,
	};

	CHECK_RESULT(device.createPipelineLayout(&info, GetAllocator(), &pipeline_layout));
}

void MainAppImpl::CreateVertexShaderModule() {
	// std::optional<std::vector<std::byte>> shader_code = Utils::ReadBinaryFile(Constants::kVertexShaderPath);
	// if (!shader_code.has_value()) {
	// 	LOG_FATAL("Failed to read vertex shader file %s!", Constants::kVertexShaderPath.data());
	// }

	std::span<u32 const> shader_code = kQuadVertexShaderCode;

	vk::ShaderModuleCreateInfo info{
		.codeSize = shader_code.size() * sizeof(shader_code[0]),
		.pCode    = reinterpret_cast<const u32*>(shader_code.data()),
	};
	CHECK_RESULT(device.createShaderModule(&info, GetAllocator(), &vertex_shader_module));
}

// auto MainAppImpl::CreatePipeline(std::string_view const fragment_shader_code) -> vk::Result {
auto MainAppImpl::CreatePipeline(std::span<std::byte const> fragment_shader_code) -> vk::Result {
	vk::Result result;

	vk::ShaderModuleCreateInfo shader_module_info{
		.codeSize = fragment_shader_code.size() * sizeof(fragment_shader_code[0]),
		.pCode    = reinterpret_cast<const u32*>(fragment_shader_code.data()),
	};

	struct ShaderModuleRAII : vk::ShaderModule {
		ShaderModuleRAII(vk::Device device, vk::AllocationCallbacks const* allocator)
			: vk::ShaderModule{}, device(device), allocator(allocator) {}
		ShaderModuleRAII(ShaderModuleRAII const& other)            = delete;
		ShaderModuleRAII& operator=(vk::ShaderModule const& other) = delete;
		ShaderModuleRAII(ShaderModuleRAII&& other)
			: vk::ShaderModule{std::exchange(static_cast<vk::ShaderModule&>(other), vk::ShaderModule{})},
			  device(std::move(other.device)),
			  allocator(std::move(other.allocator)) {}
		ShaderModuleRAII& operator=(ShaderModuleRAII&& other) {
			vk::ShaderModule::operator=(std::exchange(static_cast<vk::ShaderModule&>(other), vk::ShaderModule{}));
			device    = std::move(other.device);
			allocator = std::move(other.allocator);
			return *this;
		}
		~ShaderModuleRAII() {
			if (vk::ShaderModule::operator bool()) {
				device.destroyShaderModule(*this, allocator);
				vk::ShaderModule::operator=(vk::ShaderModule{});
			}
		}
		vk::Device                     device;
		vk::AllocationCallbacks const* allocator;
	};

	ShaderModuleRAII fragment_shader_module(device, GetAllocator());

	result = (device.createShaderModule(&shader_module_info, GetAllocator(), &fragment_shader_module));
	if (result != vk::Result::eSuccess) {
		return result;
	}

	vk::PipelineShaderStageCreateInfo shader_stages[] = {
		{.stage = vk::ShaderStageFlagBits::eVertex, .module = vertex_shader_module, .pName = "main"},
		{.stage = vk::ShaderStageFlagBits::eFragment, .module = fragment_shader_module, .pName = "main"},
	};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state{};

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{
		.flags                  = {},
		.topology               = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = vk::False,
	};

	vk::PipelineViewportStateCreateInfo viewport_state{
		.viewportCount = 1,
		.scissorCount  = 1,
	};

	vk::PipelineRasterizationStateCreateInfo rasterization_state{
		// .cullMode  = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eCounterClockwise,
		.lineWidth = 1.0f,
	};

	vk::PipelineMultisampleStateCreateInfo multisample_state{
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
	};

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{
		.depthTestEnable  = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp   = vk::CompareOp::eLess,
		.minDepthBounds   = 0.0f,
		.maxDepthBounds   = 1.0f,
	};

	vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
		.blendEnable    = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR |
						  vk::ColorComponentFlagBits::eG |
						  vk::ColorComponentFlagBits::eB |
						  vk::ColorComponentFlagBits::eA,
	};

	vk::PipelineColorBlendStateCreateInfo color_blend_state{
		.attachmentCount = 1,
		.pAttachments    = &color_blend_attachment_state,
	};

	vk::DynamicState dynamic_states[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	vk::PipelineDynamicStateCreateInfo dynamic_state{
		.dynamicStateCount = static_cast<u32>(std::size(dynamic_states)),
		.pDynamicStates    = dynamic_states,
	};

	vk::PipelineRenderingCreateInfo pipeline_rendering_info{
		.viewMask                = 0,
		.colorAttachmentCount    = 1,
		.pColorAttachmentFormats = &swapchain.GetFormat(),
		// .depthAttachmentFormat   = vk::Format::eD32Sfloat,
		// .stencilAttachmentFormat = vk::Format::eUndefined,
	};

	vk::GraphicsPipelineCreateInfo info{
		.pNext               = &pipeline_rendering_info,
		.stageCount          = static_cast<u32>(std::size(shader_stages)),
		.pStages             = shader_stages,
		.pVertexInputState   = &vertex_input_state,
		.pInputAssemblyState = &input_assembly_state,
		.pViewportState      = &viewport_state,
		.pRasterizationState = &rasterization_state,
		.pMultisampleState   = &multisample_state,
		// .pDepthStencilState  = &depth_stencil_state,
		.pColorBlendState = &color_blend_state,
		.pDynamicState    = &dynamic_state,
		.layout           = pipeline_layout,
	};

	result = (device.createGraphicsPipelines(GetPipelineCache(), 1, &info, GetAllocator(), &pipeline));
	if (result != vk::Result::eSuccess) {
		return result;
	}

	return vk::Result::eSuccess;
}

void MainAppImpl::DrawWindow() {
	auto HandleSwapchainResult = [this](vk::Result result) -> bool {
		switch (result) {
		case vk::Result::eSuccess:           return true;
		case vk::Result::eErrorOutOfDateKHR: swapchain_dirty = true; return false;
		case vk::Result::eSuboptimalKHR:     swapchain_dirty = true; return true;
		default:
			CHECK_RESULT(result);
		}
		return false;
	};
	CHECK_RESULT(device.waitForFences(1, &swapchain.GetCurrentFence(), vk::True, std::numeric_limits<u32>::max()));
	CHECK_RESULT(device.resetFences(1, &swapchain.GetCurrentFence()));
	device.resetCommandPool(swapchain.GetCurrentCommandPool());
	if (!HandleSwapchainResult(swapchain.AcquireNextImage())) return;
	RecordCommands();
	if (!HandleSwapchainResult(swapchain.SubmitAndPresent(queue, queue))) return;
}

void MainAppImpl::RecordCommands() {
	int x, y, width, height;
	window.GetRect(x, y, width, height);

	vk::Rect2D               render_rect{0, 0, static_cast<u32>(width), static_cast<u32>(height)};
	VulkanRHI::CommandBuffer cmd = swapchain.GetCurrentCommandBuffer();
	CHECK_RESULT(cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit}));
	vk::Image swapchain_image = swapchain.GetCurrentImage();
	// cmd.SetViewport({0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f});
	cmd.SetViewport({0.0f, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), 0.0f, 1.0f}); // flip
	cmd.SetScissor(render_rect);
	cmd.Barrier({
		.image         = swapchain_image,
		.aspectMask    = vk::ImageAspectFlagBits::eColor,
		.oldLayout     = vk::ImageLayout::eUndefined,
		.newLayout     = vk::ImageLayout::eColorAttachmentOptimal,
		.srcStageMask  = vk::PipelineStageFlagBits2::eNone,
		.srcAccessMask = vk::AccessFlagBits2::eNone,
		.dstStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
	});
	cmd.BeginRendering({
		.renderArea       = render_rect,
		.colorAttachments = {{{
			.imageView   = swapchain.GetCurrentImageView(),
			.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		}}},
	});
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	// cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
	PushConstants constants{
		.resolution = {static_cast<float>(width), static_cast<float>(height)},
		.mouse      = {mouse.x, static_cast<float>(height) - mouse.y},
	};
	cmd.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);
	// vk::DeviceSize offsets[] = {0};
	// cmd.bindVertexBuffers(0, 1, &vertex_buffer, offsets);
	cmd.draw(6, 1, 0, 0);
	cmd.endRendering();
	cmd.Barrier({
		.image         = swapchain_image,
		.aspectMask    = vk::ImageAspectFlagBits::eColor,
		.oldLayout     = vk::ImageLayout::eColorAttachmentOptimal,
		.newLayout     = vk::ImageLayout::ePresentSrcKHR,
		.srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
		.dstStageMask  = vk::PipelineStageFlagBits2::eNone,
		.dstAccessMask = vk::AccessFlagBits2::eNone,
	});
	CHECK_RESULT(cmd.end());
}

void MainAppImpl::RecreateSwapchain(int width, int height) {
	for (auto& frame : swapchain.GetFrameData()) {
		CHECK_RESULT(device.waitForFences(1, &frame.GetFence(), vk::True, std::numeric_limits<u32>::max()));
	}
	CHECK_RESULT(swapchain.Recreate(width, height));
	swapchain_dirty = false;
	// std::printf("Recr with size %dx%d\n", width, height);
}

void PrintUsage(char const* app_name) {
	std::printf("Usage: %ls <path_to_shader> [--validation] [--verbose]\n", (std::filesystem::path(app_name)).filename().c_str());
}

auto MainAppImpl::ParseArgs(int argc, char const* const* argv) -> char const* {
	for (std::string_view const arg : std::span(argv + 2, argc - 2)) {
		if (arg == "--validation") bValidationEnabled = false;
		else if (arg == "--verbose") bVerbose = true;
		else return arg.data();
	}
	return nullptr;
}

void MainAppImpl::Run(int argc, char const* const* argv) {
	if (argc < 2) {
		PrintUsage(argv[0]);
		return;
	}
	fragment_shader_path = argv[1];
	if (char const* unknown_arg = ParseArgs(argc, argv); unknown_arg) {
		std::printf("Unknown argument: %s\n", unknown_arg);
		PrintUsage(argv[0]);
		return;
	}
	// std::filesystem::current_path(std::filesystem::absolute(argv[0]).parent_path());
	Init();
	MainLoop();
}

void MainAppImpl::MainLoop() {
	do {
		WindowManager::WaitEvents();
		if (window.GetShouldClose()) break;
		int x, y, width, height;
		window.GetRect(x, y, width, height);
		if (width <= 0 || height <= 0) continue;
		DrawWindow();
	} while (true);
};

void MainApplication::Run(int argc, char** argv) {
	MainAppImpl app;
	gAppData = &app;
	app.Run(argc, argv);
}
