module;
#include <GLFW/glfw3.h>

#if 0
#include <cstdio>
#define LOG_WINDOW(fmt, ...) std::printf(fmt "\n", ##__VA_ARGS__);
#define LOG_INPUT(fmt, ...) std::printf(fmt "\n", ##__VA_ARGS__);
#else
#define LOG_WINDOW(fmt, ...)
#define LOG_INPUT(fmt, ...)
#endif

module Window;

import WindowCreateInfo;

Window::Window() {}

void Window::Init(WindowCreateInfo const& info) {
	GLFWmonitor* monitor      = nullptr;
	this->mode                = info.mode;
	int width                 = info.width;
	int height                = info.height;
	windowedDimensions.width  = width;
	windowedDimensions.height = height;
	if (info.mode == WindowMode::eWindowedFullscreen) {
		monitor        = glfwGetPrimaryMonitor();
		auto videoMode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);
		width  = videoMode->width;
		height = videoMode->height;
	} else if (info.mode == WindowMode::eFullscreen) {
		monitor = glfwGetPrimaryMonitor();
	}

	glfwWindowHint(GLFW_RESIZABLE, info.bResizable);
	glfwWindowHint(GLFW_DECORATED, info.bDecorated);
	glfwWindowHint(GLFW_FOCUSED, info.bFocused);
	glfwWindowHint(GLFW_AUTO_ICONIFY, info.bAutoIconify);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, info.bTransparent);
	glfwWindowHint(GLFW_FLOATING, info.bFloating);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, info.title, monitor, nullptr);
	glfwSetWindowUserPointer(window, this);
	// Set pos from info
	int x, y;
	glfwGetWindowPos(window, &x, &y);
	if (info.x != kWindowDontCare) {
		x                    = info.x;
		windowedDimensions.x = info.x;
	}
	if (info.y != kWindowDontCare) {
		y                    = info.y;
		windowedDimensions.y = info.y;
	}
	if (x != kWindowDontCare || y != kWindowDontCare) {
		glfwSetWindowPos(window, x, y);
	}

	// Get Stored Rect
	// glfwGetWindowPos(window, &windowedDimensions.x, &windowedDimensions.y);
	// glfwGetWindowSize(window, &windowedDimensions.width, &windowedDimensions.height);
}

Window::~Window() {
	Destroy();
}

void Window::SetPos(int x, int y) {
	glfwSetWindowPos(window, x, y);
}

void Window::SetSize(int width, int height) {
	glfwSetWindowSize(window, width, height);
}

bool Window::GetFullScreenRect(int& x, int& y, int& width, int& height) const {
	auto monitor   = glfwGetPrimaryMonitor();
	auto videoMode = glfwGetVideoMode(monitor);
	glfwGetMonitorPos(monitor, &x, &y);
	width  = videoMode->width;
	height = videoMode->height;
	return true;
}

void Window::SetSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight) {
	auto TranslateWindowDontCare = [](int value) {
		if (value == kWindowDontCare) {
			return GLFW_DONT_CARE;
		}
		return value;
	};
	glfwSetWindowSizeLimits(window, TranslateWindowDontCare(minWidth), TranslateWindowDontCare(minHeight),
							TranslateWindowDontCare(maxWidth), TranslateWindowDontCare(maxHeight));
}

void Window::Destroy() {
	if (window) {
		glfwDestroyWindow(window);
		window = nullptr;
	}
}

void Window::Minimize() {
	glfwIconifyWindow(window);
}

void Window::Maximize() {
	glfwMaximizeWindow(window);
}

void Window::Restore() {
	glfwRestoreWindow(window);
}

void Window::Show() {
	glfwShowWindow(window);
}

void Window::Hide() {
	glfwHideWindow(window);
}

void Window::SetWindowMode(WindowMode mode) {
	auto monitor   = glfwGetPrimaryMonitor();
	auto videoMode = glfwGetVideoMode(monitor);
	switch (mode) {
	case WindowMode::eWindowed:
		glfwSetWindowMonitor(window, nullptr, windowedDimensions.x, windowedDimensions.y, windowedDimensions.width, windowedDimensions.height, GLFW_DONT_CARE);
		break;
	case WindowMode::eWindowedFullscreen:
		glfwGetWindowPos(window, &windowedDimensions.x, &windowedDimensions.y);
		glfwGetWindowSize(window, &windowedDimensions.width, &windowedDimensions.height);
		glfwSetWindowMonitor(window, monitor, 0, 0, videoMode->width, videoMode->height, videoMode->refreshRate);
		break;
	case WindowMode::eFullscreen:
		glfwGetWindowPos(window, &windowedDimensions.x, &windowedDimensions.y);
		glfwGetWindowSize(window, &windowedDimensions.width, &windowedDimensions.height);
		int x, y, width, height;
		GetRect(x, y, width, height);
		glfwSetWindowMonitor(window, monitor, 0, 0, width, height, videoMode->refreshRate);
		break;
	case WindowMode::eMaxEnum: break;
	}
	this->mode = mode;
}

auto Window::GetWindowMode() const -> WindowMode {
	return mode;
}

bool Window::IsMaximized() const {
	return glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
}

bool Window::IsMinimized() const {
	return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}

bool Window::GetRestoredRect(int& x, int& y, int& width, int& height) const {
	if (mode == WindowMode::eWindowed) {
		GetRect(x, y, width, height);
		return true;
	}
	x      = windowedDimensions.x;
	y      = windowedDimensions.y;
	width  = windowedDimensions.width;
	height = windowedDimensions.height;
	return true;
}

void Window::GetRect(int& x, int& y, int& width, int& height) const {
	glfwGetWindowPos(window, &x, &y);
	glfwGetWindowSize(window, &width, &height);
}

void Window::SetFocus() {
	glfwFocusWindow(window);
}

void Window::SetOpacity(float const value) {
	glfwSetWindowOpacity(window, value);
}

void Window::Enable(bool bEnable) {
	glfwSetInputMode(window, GLFW_CURSOR, bEnable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

auto Window::GetHandle() const -> void* {
	return window;
}

void Window::SetText(const char* const text) {
	glfwSetWindowTitle(window, text);
}
