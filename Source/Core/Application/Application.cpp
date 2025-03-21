module;
#include "Log/LogMacros.hpp"
#include "Misc/CheckVulkanResult.hpp"
#include "Shaders/PushConstants.h"
module Application;
import std;
import vulkan_hpp;
import VulkanRHI;
import MessageCallbacks;
import Window;
import WindowManager;
import WindowState;
import Utils;
import VulkanExtensions;
import ShaderCodes;
import ShaderCompiler;
import FileManager;
import ApplicationGlobalData;
import ParseUtils;
import Log;
import Glfw;

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

struct FragmentShaderManager {
	void Update(std::string_view const fragment_shader_code) {
		path             = fragment_shader_code;
		path_string      = path.string();
		file_version     = Utils::GetFileVersion(path_string);
		pipeline_version = -1;
	}

	bool UpdateFileVersion() {
		int new_version = Utils::GetFileVersion(path_string);
		if (new_version > file_version) {
			file_version = new_version;
			return true;
		}
		return false;
	}

	bool IsUpToDate(int current_file_version) const { return file_version >= current_file_version; }

	auto GetPath() -> std::filesystem::path& { return path; }
	auto GetPath() const -> std::filesystem::path const& { return path; }
	auto GetPathString() -> std::string& { return path_string; }
	auto GetPathString() const -> std::string const& { return path_string; }
	auto GetFileVersion() -> int& { return file_version; }
	auto GetFileVersion() const -> int const& { return file_version; }
	auto GetPipelineVersion() -> int& { return pipeline_version; }
	auto GetPipelineVersion() const -> int const& { return pipeline_version; }
	bool GetDirty() const { return GetPipelineVersion() < GetFileVersion(); }

	void SetFileVersion(int new_version) { file_version = new_version; }
	void SetPipelineVersion(int new_version) { pipeline_version = new_version; }

	std::filesystem::path path;
	std::string           path_string;
	int                   file_version     = -1;
	int                   pipeline_version = -1;
};

constexpr float kFpsUnlimited = 0.0f;

struct UserOptions {
	bool  bVerbose : 1           = false;
	bool  bFlipY : 1             = true;
	bool  bUpdateOnSave : 1      = true;
	bool  bValidationEnabled : 1 = true;
	bool  bStartPaused : 1       = false;
	bool  bTransparent : 1       = false;
	float fps_limit              = -1.0f;

	std::string_view compile_options = "";
};

struct KeyboardAction {
	Glfw::Key    key;
	Glfw::Action action;
	Glfw::Mod    mods;

	bool operator==(KeyboardAction const& other) const noexcept {
		return key == other.key && action == other.action && mods == other.mods;
	}
};

template <>
struct std::hash<KeyboardAction> {
	std::size_t operator()(KeyboardAction const& key) const noexcept {
		std::size_t hash = 0;
		Utils::HashCombine(hash, std::to_underlying(key.key));
		Utils::HashCombine(hash, std::to_underlying(key.action));
		Utils::HashCombine(hash, std::to_underlying(key.mods));
		return hash;
	}
};

struct MainAppImpl {
	static constexpr u32 kApiVersion          = vk::ApiVersion13;
	static constexpr int kDefaultWindowWidth  = 800;
	static constexpr int kDefaultWindowHeight = 600;

	static constexpr char const* kEnabledLayers[]           = {"VK_LAYER_KHRONOS_validation"};
	static constexpr char const* kEnabledDeviceExtensions[] = {
		vk::KHRSwapchainExtensionName,
	};

	~MainAppImpl();
	void Run(int argc, char const* const* argv);
	void Init();
	void MainLoop();
	void WaitForFrameTimeLeft();
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
	void CreateFallbackPipeline();
	bool UpdateUserFragmentShader();

	[[nodiscard]] auto CreatePipeline(std::span<std::byte const> fragment_shader_code, vk::Pipeline& pipeline) -> vk::Result;
	[[nodiscard]] bool TryRecreateUserPipeline();

	void RecordCommands();
	void UpdateViewport(int width, int height);
	void UpdateMouse(int x, int y, int height);
	void OnDrawWindow();
	void UpdateTime();
	void RecreateSwapchain(int width, int height);

	auto GetAllocator() const -> vk::AllocationCallbacks const* { return allocator; }
	auto GetPipelineCache() const -> vk::PipelineCache { return pipeline_cache; }

	bool CallKeyCallback(KeyboardAction const& key);
	void SetupKeyCallbackMap();

	template <typename... Args>
	void LogVerbose(Args&&... args) const;

	struct UserOptions user_options;

	ShaderCompiler        shader_compiler;
	FileManager           file_manager;
	FragmentShaderManager fragment_shader;

	Window       window;
	WindowState  window_state;
	int          WindowFramesToDraw = 0;
	vk::Viewport viewport;
	vk::Viewport viewport_flip_y;
	struct {
		float x = 300.0f;
		float y = 300.0f;

		Glfw::Action button_state[std::underlying_type_t<Glfw::MouseButton>(Glfw::MouseButton::eLast) + 1];
	} mouse;

	// startup time
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> last_time  = std::chrono::high_resolution_clock::now();

	long long total_shader_time_mks = 0;

	float time = 0.0f;
	float time_delta;
	int   frame_index = 0;

	bool bPaused = false;

	vk::Instance                    instance{};
	vk::AllocationCallbacks const*  allocator{nullptr};
	vk::PipelineCache               pipeline_cache{nullptr};
	vk::DebugUtilsMessengerEXT      debug_messenger{};
	std::span<char const* const>    enabled_layers{};
	std::vector<vk::PhysicalDevice> vulkan_physical_devices{};
	PhysicalDevice                  physical_device{};
	vk::Device                      device{};
	VulkanRHI::Swapchain            swapchain{};
	bool                            bSwapchainDirty = false;
	vk::SurfaceKHR                  surface{};

	vk::Queue queue{};
	u32       queue_family_index = ~0u;

	// vk::DescriptorSetLayout descriptor_set_layout{};
	// vk::DescriptorPool      descriptor_pool{};
	// vk::DescriptorSet       descriptor_set{};

	vk::PipelineLayout pipeline_layout;

	// Owning
	vk::Pipeline  user_pipeline{};
	vk::Pipeline  fallback_pipeline;
	vk::Pipeline* current_pipeline;

	vk::ShaderModule vertex_shader_module;
	vk::ShaderModule fragment_shader_module;
};

std::unordered_map<KeyboardAction, void (*)(MainAppImpl*)> callback_map;

static MainAppImpl* gApp = nullptr;

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
	gApp->bSwapchainDirty = true;
	if (width <= 0 || height <= 0) return;
	gApp->UpdateViewport(width, height);
	gApp->RecreateSwapchain(width, height);
}

static void WindowRefreshCallback(GLFWwindow* window) {
	gApp->OnDrawWindow();
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	using namespace Glfw;
	Window* app_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	int     x, y, width, height;
	app_window->GetRect(x, y, width, height);
	float  dx          = xpos - gApp->mouse.x;
	float  dy          = ypos - gApp->mouse.y;
	Action right_state = gApp->mouse.button_state[std::underlying_type_t<MouseButton>(MouseButton::eRight)];
	if (right_state == Action::ePress || right_state == Action::eRepeat) [[unlikely]] {
		gApp->window.SetPos(x + dx, y + dy);
		gApp->mouse.x = static_cast<float>(xpos - dx);
		gApp->mouse.y = static_cast<float>(ypos - dy);
	} else {
		gApp->mouse.x = static_cast<float>(xpos);
		gApp->mouse.y = static_cast<float>(ypos);
	}
}

bool MainAppImpl::CallKeyCallback(KeyboardAction const& key) {
	auto it = callback_map.find(key);
	if (it != callback_map.end()) {
		it->second(gApp);
		return true;
	}
	return false;
}

static void KeyCallback(GLFWwindow* in_window, int in_keycode, int in_scancode, int in_action, int in_mods) {
	using namespace Glfw;
	[[maybe_unused]] bool bConsumed = gApp->CallKeyCallback({Key(in_keycode), Action(in_action), Mod(in_mods)});
}

static void MouseButtonCallback(GLFWwindow* in_window, int in_button, int in_action, int in_mods) {
	using namespace Glfw;
	MouseButton button = static_cast<MouseButton>(in_button);
	Action      action = static_cast<Action>(in_action);
	Mod         mods   = static_cast<Mod>(in_mods);

	gApp->mouse.button_state[in_button] = action;
}

void MainAppImpl::Init() {
	if (user_options.bStartPaused) {
		bPaused = true;
	}
	WindowManager::SetErrorCallback(WindowErrorCallback);
	WindowManager::Init();
	char        title_buffer[256];
	std::size_t pos = fragment_shader.path_string.find_last_of('/');
	char const* file_name =
		pos == std::string::npos
			? fragment_shader.path_string.data()
			: fragment_shader.path_string.data() + pos + 1;

	std::snprintf(title_buffer, sizeof(title_buffer) - 1, "%s - %s",
				  file_name,
				  gGlobalData.application_title.data());

	WindowMode    initial_window_mode = WindowMode::eWindowed;
	WindowRect    initial_window_rect{.x = kWindowDontCare, .y = kWindowDontCare, .width = kDefaultWindowWidth, .height = kDefaultWindowHeight};
	constexpr int kMinWindowSizePos = 30;

	int monitor_x, monitor_y, monitor_width, monitor_height;
	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &monitor_x, &monitor_y, &monitor_width, &monitor_height);

	window_state = WindowState::FromFile(gGlobalData.window_state_path).value_or(WindowState{});
	if (window_state.x != kWindowDontCare) initial_window_rect.x = window_state.x;
	if (window_state.y != kWindowDontCare) initial_window_rect.y = window_state.y;
	if (window_state.width != kWindowDontCare) initial_window_rect.width = window_state.width;
	if (window_state.height != kWindowDontCare) initial_window_rect.height = window_state.height;
	if (window_state.mode != WindowMode::eMaxEnum) initial_window_mode = window_state.mode;
	// if (initial_window_rect.x < kMinWindowSizePos) initial_window_rect.x = kMinWindowSizePos;
	// if (initial_window_rect.y < kMinWindowSizePos) initial_window_rect.y = kMinWindowSizePos;
	initial_window_rect.x = std::clamp(initial_window_rect.x, kMinWindowSizePos, monitor_width - kMinWindowSizePos);
	initial_window_rect.y = std::clamp(initial_window_rect.y, kMinWindowSizePos, monitor_height - kMinWindowSizePos);
	if (initial_window_rect.width < kMinWindowSizePos) initial_window_rect.width = kMinWindowSizePos;
	if (initial_window_rect.height < kMinWindowSizePos) initial_window_rect.height = kMinWindowSizePos;

	window.Init({
		.x      = initial_window_rect.x,
		.y      = initial_window_rect.y,
		.width  = initial_window_rect.width,
		.height = initial_window_rect.height,
		.mode   = initial_window_mode,
		.title  = title_buffer,

		.bDecorated   = window_state.bDecorated,
		.bTransparent = window_state.bTransparent,
		.bFloating    = window_state.bFloating,
	});

	GLFWwindow* glfw_window_handle = reinterpret_cast<GLFWwindow*>(window.GetHandle());
	glfwSetFramebufferSizeCallback(glfw_window_handle, FramebufferSizeCallback);
	glfwSetWindowRefreshCallback(glfw_window_handle, WindowRefreshCallback);
	glfwSetCursorPosCallback(glfw_window_handle, CursorPosCallback);
	glfwSetKeyCallback(glfw_window_handle, KeyCallback);
	glfwSetMouseButtonCallback(glfw_window_handle, MouseButtonCallback);
	SetupKeyCallbackMap();

	int refresh_rate = glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate;

	if (user_options.fps_limit < 0) {
		if (refresh_rate > 0) {
			user_options.fps_limit = static_cast<float>(refresh_rate);
		} else {
			user_options.fps_limit = 60.0f;
		}
	}
	LogVerbose("Using refresh rate: %.1f", user_options.fps_limit);

	CreateInstance();

	CHECK_RESULT(WindowManager::CreateWindowSurface(instance, reinterpret_cast<GLFWwindow*>(window.GetHandle()), GetAllocator(), &surface));
	int x, y, width, height;
	window.GetRect(x, y, width, height);
	UpdateViewport(width, height);
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
	CreateFallbackPipeline();
	current_pipeline = &fallback_pipeline;
	UpdateUserFragmentShader();
}

auto FlipWindowAttrib(MainAppImpl* app, Glfw::WindowAttribute attribute) {
	auto value = glfwGetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(attribute));
	glfwSetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(attribute), !value);
};

void MainAppImpl::SetupKeyCallbackMap() {
	using namespace Glfw;
	auto PauseCallback = +[](MainAppImpl* app) {
		app->bPaused = !app->bPaused;
		LOG_INFO("Paused: %s", Utils::FormatBool(app->bPaused).data());
	};

	callback_map = {
		{KeyboardAction{Key::eD, Action::ePress, Mod::eControl}, +[](MainAppImpl* app) {
			 FlipWindowAttrib(app, WindowAttribute::eDecorated);
		 }},
		// Toggle floating window
		{KeyboardAction{Key::eF, Action::ePress, Mod::eControl}, +[](MainAppImpl* app) {
			 FlipWindowAttrib(app, WindowAttribute::eFloating);
		 }},
		{KeyboardAction{Key::eP, Action::ePress, Mod::eControl}, PauseCallback},
		{KeyboardAction{Key::eQ, Action::ePress, Mod::eControl}, PauseCallback},
		{KeyboardAction{Key::eR, Action::ePress, Mod::eControl}, +[](MainAppImpl* app) {
			 int x, y, width, height;
			 app->window.GetFullScreenRect(x, y, width, height);
			 app->window.SetPos((width - app->kDefaultWindowWidth) / 2, (height - app->kDefaultWindowHeight) / 2);
			 app->window.SetSize(app->kDefaultWindowWidth, app->kDefaultWindowHeight);
			 glfwSetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(WindowAttribute::eFloating), kFalse);
			 glfwSetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(WindowAttribute::eDecorated), kTrue);
		 }},
		//  // Toggle transparent window
		// {KeyboardAction{Key::eT, Action::ePress, Mod::eControl}, +[](MainAppImpl* app) {
		// 	 int is_transparent = glfwGetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(WindowAttribute::eTransparentFramebuffer));
		// 	 glfwSetWindowAttrib(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), std::to_underlying(WindowAttribute::eTransparentFramebuffer), !is_transparent);
		//  }},
		{KeyboardAction{Key::eY, Action::ePress, Mod::eControl}, +[](MainAppImpl* app) {
			 app->user_options.bFlipY = !app->user_options.bFlipY;
		 }},
		{KeyboardAction{Key::eEscape, Action::ePress, Mod{}}, +[](MainAppImpl* app) {
			 glfwSetWindowShouldClose(reinterpret_cast<GLFWwindow*>(app->window.GetHandle()), kTrue);
		 }},
		{KeyboardAction{Key::eF5, Action::ePress, Mod{}}, +[](MainAppImpl* app) {
			 app->fragment_shader.UpdateFileVersion();
		 }},
		{KeyboardAction{Key::eF11, Action::ePress, Mod{}}, +[](MainAppImpl* app) {
			 if (app->window.GetWindowMode() == WindowMode::eWindowed) {
				 app->window.SetWindowMode(WindowMode::eWindowedFullscreen);
			 } else {
				 app->window.SetWindowMode(WindowMode::eWindowed);
			 }
		 }},
	};
}

template <typename... Args>
void MainAppImpl::LogVerbose(Args&&... args) const {
	if (user_options.bVerbose) LOG_INFO(std::forward<Args>(args)...);
}

bool MainAppImpl::UpdateUserFragmentShader() {
	if (user_options.bUpdateOnSave) {
		fragment_shader.UpdateFileVersion();
	}
	if (fragment_shader.GetDirty()) {
		current_pipeline = &fallback_pipeline;

		static int last_recreation_attempt_file_version = -1;
		if (last_recreation_attempt_file_version != fragment_shader.GetFileVersion()) {
			last_recreation_attempt_file_version = fragment_shader.GetFileVersion();
			if (TryRecreateUserPipeline()) {
				fragment_shader.SetPipelineVersion(last_recreation_attempt_file_version);
				current_pipeline = &user_pipeline;
			}
		}
		return true;
	}
	return false;
};

MainAppImpl::~MainAppImpl() { Destroy(); }

void MainAppImpl::Destroy() {

	if (device) {
		CHECK_RESULT(device.waitIdle());

		device.destroyPipeline(user_pipeline, GetAllocator());
		device.destroyPipeline(fallback_pipeline, GetAllocator());
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
		if (user_options.bValidationEnabled) {
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
	if (user_options.bValidationEnabled) {
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
		.pNext                   = user_options.bValidationEnabled ? &kDebugUtilsCreateInfo : nullptr,
		.pApplicationInfo        = &applicationInfo,
		.enabledLayerCount       = static_cast<u32>(std::size(enabled_layers)),
		.ppEnabledLayerNames     = enabled_layers.data(),
		.enabledExtensionCount   = static_cast<u32>(std::size(enabledExtensions)),
		.ppEnabledExtensionNames = enabledExtensions.data(),
	};
	CHECK_RESULT(vk::createInstance(&info, GetAllocator(), &instance));

	if (user_options.bValidationEnabled) {
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
			if (user_options.bVerbose) {
			}
			return;
		}
	}

	LOG_FATAL("No suitable physical device found");
}

void MainAppImpl::GetPhysicalDeviceInfo() {
}

void MainAppImpl::CreateDevice() {
	float const queue_priorities[] = {1.0f};

	auto [result, index] = physical_device.GetQueueFamilyIndex({.flags = vk::QueueFlagBits::eGraphics, .surface = surface});
	if (result != vk::Result::eSuccess || !index.has_value()) {
		LOG_ERROR("Failed to get graphics queue family index with surface support");
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
		vk::PhysicalDeviceVulkan11Features{},
		vk::PhysicalDeviceVulkan12Features{},
		vk::PhysicalDeviceVulkan13Features{.synchronization2 = vk::True, .dynamicRendering = vk::True},
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
		// .preferred_format   = vk::Format::eR8G8B8A8Srgb,
		.preferred_format = vk::Format::eR8G8B8A8Unorm,
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
	std::span<u32 const> shader_code = ShaderCodes::kVertexQuad;

	vk::ShaderModuleCreateInfo info{
		.codeSize = shader_code.size() * sizeof(shader_code[0]),
		.pCode    = reinterpret_cast<const u32*>(shader_code.data()),
	};
	CHECK_RESULT(device.createShaderModule(&info, GetAllocator(), &vertex_shader_module));
}

void MainAppImpl::CreateFallbackPipeline() {
	// std::optional<std::span<std::byte const>> shader_code;
	do {
		{
			// Load from file
			std::optional<std::span<std::byte>> shader_code = file_manager.ReadBinaryFile(gGlobalData.fallback_fragment_spv_path);
			if (shader_code.has_value() && !shader_code.value().empty()) {
				LogVerbose("Loaded fallback fragment shader from file %s.", gGlobalData.fallback_fragment_spv_path.data());
				vk::Result result = CreatePipeline(shader_code.value(), fallback_pipeline);
				if (result == vk::Result::eSuccess) continue;
			}
		}
		LogVerbose("Failed to create fallback fragment shader from %s, compiling...", gGlobalData.fallback_fragment_shader_file_path.data());
		{
			// Compile and load
			if (shader_compiler.CompileShader(gGlobalData.fallback_fragment_shader_file_path, gGlobalData.fallback_fragment_spv_path, user_options.compile_options)) {
				std::optional<std::span<std::byte const>> shader_code = file_manager.ReadBinaryFile(gGlobalData.fallback_fragment_spv_path);
				if (shader_code.has_value() && !shader_code.value().empty()) {
					LogVerbose("Loaded compiled fallback fragment.");
					vk::Result result = CreatePipeline(shader_code.value(), fallback_pipeline);
					if (result == vk::Result::eSuccess) continue;
				} else {
					LogVerbose("Failed to load compiled fallback fragment.");
				}
			} else {
				LogVerbose("Error while compiling fallback fragment shader: %s", shader_compiler.GetErrorMessage().data());
			}
		}
		LogVerbose("Failed to create fallback pipeline, falling back to basic fragment shader.");
		{
			// Create from binary array code
			std::span<const std::byte> fallback_fragment_shader_code{
				reinterpret_cast<const std::byte*>(ShaderCodes::kFragmentFallbackDefault),
				sizeof(ShaderCodes::kFragmentFallbackDefault),
			};
			CHECK_RESULT(CreatePipeline(fallback_fragment_shader_code, fallback_pipeline));
		}
	} while (false);
};

auto MainAppImpl::CreatePipeline(std::span<std::byte const> fragment_shader_code, vk::Pipeline& pipeline) -> vk::Result {
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
	if (result != vk::Result::eSuccess) [[unlikely]] {
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

bool MainAppImpl::TryRecreateUserPipeline() {
	std::chrono::high_resolution_clock::time_point compile_start_time = std::chrono::high_resolution_clock::now();
	if (!shader_compiler.CompileShader(fragment_shader.path_string, gGlobalData.user_fragment_spv_path, user_options.compile_options)) return false;
	std::chrono::duration<double>             compile_time    = std::chrono::high_resolution_clock::now() - compile_start_time;
	auto                                      compile_time_ms = compile_time.count() * 1000.0f;
	std::optional<std::span<const std::byte>> shader_code     = file_manager.ReadBinaryFile(gGlobalData.user_fragment_spv_path);
	if (!shader_code.has_value()) {
		LOG_ERROR("Error: %s", shader_compiler.GetErrorMessage().data());
		return false;
	}
	if (shader_code.value().empty()) {
		LOG_ERROR("Compiled fragment shader file is empty.");
		return false;
	}

	std::chrono::high_resolution_clock::time_point pipeline_start_time = std::chrono::high_resolution_clock::now();

	vk::Pipeline                  new_pipeline;
	vk::Result                    result           = CreatePipeline(shader_code.value(), new_pipeline);
	std::chrono::duration<double> pipeline_time    = std::chrono::high_resolution_clock::now() - pipeline_start_time;
	auto                          pipeline_time_ms = pipeline_time.count() * 1000.0f;
	// CHECK_RESULT(result);
	if (result != vk::Result::eSuccess) {
		return false;
	}
	CHECK_RESULT(device.waitIdle());
	device.destroyPipeline(user_pipeline, GetAllocator());
	user_pipeline = new_pipeline;
	LogVerbose("Updated shader %s. Compilation time: %.3f ms. Pipeline creation time: %.3f ms",
			   fragment_shader.path_string.data(), compile_time_ms, pipeline_time_ms);
	return true;
};

void MainAppImpl::UpdateViewport(int width, int height) {
	viewport        = {0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
	viewport_flip_y = {0.0f, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), 0.0f, 1.0f};

	// if (user_options.bFlipY) {
	// } else {
	// }
};

void MainAppImpl::UpdateMouse(int x, int y, int height) {
	if (user_options.bFlipY) {
		mouse = {static_cast<float>(x), static_cast<float>(height) - y};
	} else {
		mouse = {static_cast<float>(x), static_cast<float>(y)};
	}
}

void MainAppImpl::UpdateTime() {
	auto      now    = std::chrono::high_resolution_clock::now();
	auto      d_time = now - last_time;
	long long d_ms   = std::chrono::duration_cast<std::chrono::milliseconds>(d_time).count();
	last_time        = now;

	time_delta = static_cast<double>(d_ms) / 1'000.0;
	total_shader_time_mks += d_ms;
	time = static_cast<double>(total_shader_time_mks) / 1'000.0;
	// time       = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count()) / 1000.0;
	// LogVerbose("Time: %f, delta: %f", time, time_delta);
}

void MainAppImpl::OnDrawWindow() {
	auto HandleSwapchainResult = [this](vk::Result result) -> bool {
		switch (result) {
		case vk::Result::eSuccess:           return true;
		case vk::Result::eErrorOutOfDateKHR: bSwapchainDirty = true; return false;
		case vk::Result::eSuboptimalKHR:     bSwapchainDirty = true; return true;
		default:
			CHECK_RESULT(result);
		}
		return false;
	};

	int x, y, width, height;
	window.GetRect(x, y, width, height);
	if (width <= 0 || height <= 0) return;
	CHECK_RESULT(device.waitForFences(1, &swapchain.GetCurrentFence(), vk::True, std::numeric_limits<u32>::max()));
	CHECK_RESULT(device.resetFences(1, &swapchain.GetCurrentFence()));
	device.resetCommandPool(swapchain.GetCurrentCommandPool());
	if (!HandleSwapchainResult(swapchain.AcquireNextImage())) return;
	if (!bPaused) {
		UpdateTime();
	}
	RecordCommands();
	if (!HandleSwapchainResult(swapchain.SubmitAndPresent(queue, queue))) return;
	// LogVerbose("Window drawn");
	++frame_index;
}

void MainAppImpl::RecordCommands() {
	int x, y, width, height;
	window.GetRect(x, y, width, height);
	vk::Rect2D render_rect{0, 0, static_cast<u32>(width), static_cast<u32>(height)};

	VulkanRHI::CommandBuffer cmd = swapchain.GetCurrentCommandBuffer();
	CHECK_RESULT(cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit}));
	vk::Image swapchain_image = swapchain.GetCurrentImage();
	if (user_options.bFlipY || current_pipeline == &fallback_pipeline) {
		cmd.setViewport(0, {viewport_flip_y});
	} else {
		cmd.setViewport(0, {viewport});
	}
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
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *current_pipeline);
	// cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
	PushConstants constants{
		.resolution = {static_cast<float>(width), static_cast<float>(height)},
		// .mouse      = {mouse.x, mouse.y},
		.mouse      = {mouse.x, user_options.bFlipY ? height - mouse.y : mouse.y},
		.time       = time,
		.time_delta = time_delta,
		.frame      = frame_index,
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
	bSwapchainDirty = false;
	// std::printf("Recr with size %dx%d\n", width, height);
}

static std::chrono::high_resolution_clock::time_point last_frame_time = std::chrono::high_resolution_clock::now();

void MainAppImpl::WaitForFrameTimeLeft() {
	auto fps_limit = user_options.fps_limit;
	if (fps_limit == kFpsUnlimited || fps_limit == 0) return;
	auto now             = std::chrono::high_resolution_clock::now();
	auto elapsed         = now - last_time;
	auto frame_time_left = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<float>(1.0f / fps_limit) - elapsed);
	if (frame_time_left > std::chrono::milliseconds(0)) {
		std::this_thread::sleep_for(frame_time_left);
	}
	last_time = now;
}

void MainAppImpl::MainLoop() {
	do {
		WindowManager::PollEvents();
		// if (bPaused) {
		// 	WindowManager::WaitEvents();
		// } else {
		// }
		// WindowManager::WaitEventsTimeout(0.1);
		if (glfwWindowShouldClose(reinterpret_cast<GLFWwindow*>(window.GetHandle()))) [[unlikely]]
			break;
		bool bUpdated = UpdateUserFragmentShader();
		if (!bPaused || bUpdated) {
			OnDrawWindow();
		};
		WaitForFrameTimeLeft();
	} while (true);
};

struct ArgParser {
	ArgParser(int argc, char const* const* argv, UserOptions& user_options) : argv(argv), argc(argc), user_options(&user_options) {}
	auto Parse() -> char const*;

	auto GetNextArg() -> std::string_view {
		if (curent_arg >= argc) return {};
		return argv[curent_arg++];
	}
	auto IsBoolString(std::string_view const value) -> bool { return value == "true" || value == "false" || value == "1" || value == "0"; }
	auto IsNumString(std::string_view const value) -> bool { return value.find_first_not_of("-0123456789.") == std::string_view::npos; }
	auto ParseBoolString(std::string_view const value) -> bool { return value == "true" || value == "1"; }
	auto IsArg(const std::string_view arg) -> bool { return arg.find("--") == 0; }
	auto ParseKwargs(const std::string_view arg) -> char const*;
	auto ParseBoolKwarg(const std::string_view arg,
						const std::string_view key,
						bool&                  value) -> char const*;
	auto ParseNumKwarg(const std::string_view arg, const std::string_view key, int& value) -> char const*;

private:
	char const* const* argv;
	int                argc;
	int                curent_arg = 0;
	UserOptions*       user_options;
};

void PrintUsage() {
	UserOptions default_options{};
	std::printf("Usage: %ls <fragment_shader_file> ", gGlobalData.executable_path.filename().c_str());
	std::printf("[--help] ");
	std::printf("[--validation=%s] ", Utils::FormatBool(default_options.bValidationEnabled).data());
	std::printf("[--verbose=%s] ", Utils::FormatBool(default_options.bVerbose).data());
	std::printf("[--update-on-save=%s] ", Utils::FormatBool(default_options.bUpdateOnSave).data());
	std::printf("[--flip-y=%s] ", Utils::FormatBool(default_options.bFlipY).data());
	std::printf("[--start-paused=%s] ", Utils::FormatBool(default_options.bStartPaused).data());
	std::printf("[--transparent=%s] ", Utils::FormatBool(default_options.bTransparent).data());
	std::printf("[--fps-limit=%f] ", default_options.fps_limit);
	std::printf("[--compile_options=%s] ", default_options.compile_options.data());
	std::printf("\n");
}

void PrintHelp() {
	PrintUsage();
	std::printf("  --validation=<bool>   Enable Vulkan validation layers\n");
	std::printf("  --verbose=<bool>      Enable verbose logging\n");
	std::printf("  --update-on-save=<bool> Update the shader on save\n");
	std::printf("  --flip-y=<bool>       Flip the Y axis\n");
	std::printf("  --start-paused=<bool> Start paused\n");
	std::printf("  --transparent=<bool>  Make the window transparent\n");
	std::printf("  --fps-limit=<float>   FPS limit. Use monitor refresh rate by default. Disable with 0\n");

	std::printf("  --compile_options=<string> Options for shader compilation\n");
}

auto ArgParser::ParseBoolKwarg(const std::string_view arg, const std::string_view key, bool& value) -> char const* {
	std::string_view value_str;
	if (arg == key) {
		value_str = "1";
	} else if (arg.find(key) == 0 && arg.size() > key.size() && arg[key.size()] == '=') {
		value_str = arg.substr(key.size() + 1);
	}
	if (value_str.empty() || !IsBoolString(value_str)) return arg.data();
	value = ParseBoolString(value_str);
	return nullptr;
};

auto ArgParser::ParseNumKwarg(const std::string_view arg, const std::string_view key, int& value) -> char const* {
	std::string_view value_str;
	if (arg == key) {
		value_str = "1";
	} else if (arg.find(key) == 0 && arg.size() > key.size() && arg[key.size()] == '=') {
		value_str = arg.substr(key.size() + 1);
	}
	if (value_str.empty() /*  || !IsNumString(value_str) */) return arg.data();
	char* p_end{nullptr};
	long  value_long = std::strtol(value_str.data(), &p_end, 10);
	if (p_end == value_str.data() ||
		*p_end != '\0' ||
		value_long == std::numeric_limits<long>::min() ||
		value_long == std::numeric_limits<long>::max()) return arg.data();
	value = static_cast<int>(value_long);
	return nullptr;
}

auto ArgParser::ParseKwargs(const std::string_view arg) -> char const* {
	bool value;
	int  value_int;
	if (!ParseBoolKwarg(arg, "--validation", value)) user_options->bValidationEnabled = value;
	else if (!ParseBoolKwarg(arg, "--verbose", value)) user_options->bVerbose = value;
	else if (!ParseBoolKwarg(arg, "--update-on-save", value)) user_options->bUpdateOnSave = value;
	else if (!ParseBoolKwarg(arg, "--flip-y", value)) user_options->bFlipY = value;
	else if (!ParseBoolKwarg(arg, "--start-paused", value)) user_options->bStartPaused = value;
	else if (!ParseBoolKwarg(arg, "--transparent", value)) user_options->bTransparent = value;
	else if (!ParseNumKwarg(arg, "--fps-limit", value_int)) {
		if (value_int < 0) value_int = -1;
		user_options->fps_limit = static_cast<float>(value_int);
	} else if (Utils::ParseString(arg, "--compile_options=", user_options->compile_options)) {
	} else return arg.data();
	return nullptr;
}

auto ArgParser::Parse() -> char const* {
	while (true) {
		std::string_view const arg = GetNextArg();
		if (arg.empty()) return nullptr;
		// if (ParseFlag(arg) == nullptr) continue;
		char const* unknown_arg = ParseKwargs(arg);
		if (unknown_arg) return unknown_arg;
	}
	return nullptr;
}

void MainAppImpl::Run(int argc, char const* const* argv) {
	gGlobalData.executable_path        = std::filesystem::absolute(argv[0]);
	gGlobalData.executable_path_string = gGlobalData.executable_path.string();
	gGlobalData.executable_dir         = gGlobalData.executable_path.parent_path();
	gGlobalData.executable_dir_string  = gGlobalData.executable_dir.string();
	gGlobalData.config_dir             = gGlobalData.executable_dir_string.data();
	gGlobalData.temp_dir               = std::filesystem::temp_directory_path();
	gGlobalData.temp_dir_string        = gGlobalData.temp_dir.string();
	// gGlobalData.temp_dir                   = ".";
	// gGlobalData.temp_dir_string            = ".";
	gGlobalData.window_state_path          = gGlobalData.config_dir + "/WindowState.ini";
	gGlobalData.fallback_fragment_spv_path = gGlobalData.temp_dir_string + "/Fallback.frag.spv";
	gGlobalData.user_fragment_spv_path     = gGlobalData.temp_dir_string + "/FragOutput.frag.spv";

	for (std::string_view const arg : std::span(argv + 1, argc - 1)) {
		if (arg == "--help") {
			PrintHelp();
			return;
		}
	}
	if (argc < 2) {
		LOG_ERROR("No fragment shader file specified");
		PrintUsage();
		return;
	}
	fragment_shader.Update(argv[1]);

	ArgParser arg_parser(argc - 2, argv + 2, user_options);

	if (char const* unknown_arg = arg_parser.Parse(); unknown_arg) {
		LOG_ERROR("Error in argument: %s", unknown_arg);
		PrintUsage();
		return;
	}

	if (user_options.bVerbose) {
		std::printf("UserOptions:\n");
		std::printf("  bVerbose: %s\n", Utils::FormatBool(user_options.bVerbose).data());
		std::printf("  bFlipY: %s\n", Utils::FormatBool(user_options.bFlipY).data());
		std::printf("  bUpdateOnSave: %s\n", Utils::FormatBool(user_options.bUpdateOnSave).data());
		std::printf("  bValidationEnabled: %s\n", Utils::FormatBool(user_options.bValidationEnabled).data());
		std::printf("  fps-limit: %.1f\n", user_options.fps_limit);
		std::printf("  start-paused: %s\n", Utils::FormatBool(user_options.bStartPaused).data());
		std::printf("\n");
	}

	Init();
	start_time = std::chrono::high_resolution_clock::now();
	MainLoop();

	window_state = WindowState::FromWindow(window);
	window_state.SaveToFile(gGlobalData.window_state_path);
}

void MainApplication::Run(int argc, char** argv) {
	MainAppImpl app;
	gApp = &app;
	app.Run(argc, argv);
}
