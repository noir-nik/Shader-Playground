module;
#include <GLFW/glfw3.h>
module WindowState;
import std;
import IWindow;
import Window;

static constexpr inline auto FormatBool(bool value) -> std::string_view { return value ? "true" : "false"; };

static auto ParseInt(std::string_view const line, std::string_view const key, int& value) -> bool {
	if (std::size_t pos = line.find(key); pos == 0) {
		std::size_t num_read = std::sscanf(line.data() + pos + std::size(key), "%d", &value);
		if (num_read == 1) return true;
	}
	return false;
};

static auto StripWhitespace(std::string_view const str) -> std::string_view {
	constexpr std::string_view kWhitespaces = " \t\n\r\f\v";

	return std::string_view(
		str.data() + str.find_first_not_of(kWhitespaces),
		str.data() + str.find_last_not_of(kWhitespaces) + 1);
}

static auto ParseBool(std::string_view const line, std::string_view const key, bool& value) -> bool {
	if (std::size_t pos = line.find(key); pos == 0) {
		std::string_view substr        = line.substr(pos + std::size(key));
		std::string_view stripped      = StripWhitespace(substr);
		bool             bValueIsTrue  = stripped == "true" || stripped == "1";
		bool             bValueIsFalse = stripped == "false" || stripped == "0";
		if (bValueIsTrue || bValueIsFalse) {
			value = bValueIsTrue;
			return true;
		}
	}
	return false;
};

static auto ParseString(std::string_view const line, std::string_view const key, std::string_view& value) -> bool {
	if (std::size_t pos = line.find(key); pos == 0) {
		value = StripWhitespace(line.substr(pos + std::size(key)));
		return true;
	}
	return false;
};

auto WindowState::FromWindow(Window const& window) -> WindowState {
	int x, y, width, height;
	window.GetRestoredRect(x, y, width, height);
	bool bTransparent = glfwGetWindowAttrib(reinterpret_cast<GLFWwindow*>(window.GetHandle()), GLFW_TRANSPARENT_FRAMEBUFFER);
	bool bDecorated   = glfwGetWindowAttrib(reinterpret_cast<GLFWwindow*>(window.GetHandle()), GLFW_DECORATED);
	bool bFloating    = glfwGetWindowAttrib(reinterpret_cast<GLFWwindow*>(window.GetHandle()), GLFW_FLOATING);
	return WindowState{
		.x            = x,
		.y            = y,
		.width        = width,
		.height       = height,
		.mode         = window.GetWindowMode(),
		.bDecorated   = bDecorated,
		.bTransparent = bTransparent,
		.bFloating    = bFloating};
}

bool WindowState::SaveToFile(std::string_view file_path) {
	std::FILE* config_file = std::fopen(file_path.data(), "w");
	if (config_file) {
		std::fprintf(config_file, "[WindowState]\n");
		std::fprintf(config_file, "x=%d\n", x);
		std::fprintf(config_file, "y=%d\n", y);
		std::fprintf(config_file, "width=%d\n", width);
		std::fprintf(config_file, "height=%d\n", height);
		std::fprintf(config_file, "mode=%s\n", WindowModeToString(mode));
		std::fprintf(config_file, "decorated=%d\n", bDecorated);
		std::fprintf(config_file, "transparent=%d\n", bTransparent);
		std::fprintf(config_file, "floating=%d\n", bFloating);
		std::fclose(config_file);
		return true;
	}
	return false;
}

constexpr inline auto WindowModeFromString(std::string_view str) -> WindowMode {
	if (str == "Windowed") return WindowMode::eWindowed;
	if (str == "WindowedFullscreen") return WindowMode::eWindowedFullscreen;
	if (str == "Fullscreen") return WindowMode::eFullscreen;
	return WindowMode::eWindowed;
}

[[nodiscard]] bool ParseLine(WindowState& window_state, std::string_view& line) {
	if (ParseInt(line, "x=", window_state.x) ||
		ParseInt(line, "y=", window_state.y) ||
		ParseInt(line, "width=", window_state.width) ||
		ParseInt(line, "height=", window_state.height) ||
		ParseBool(line, "decorated=", window_state.bDecorated) ||
		ParseBool(line, "transparent=", window_state.bTransparent) ||
		ParseBool(line, "floating=", window_state.bFloating)) {
		return true;
	};

	std::string_view mode;
	if (ParseString(line, "mode=", mode)) {
		if (mode.find("WindowedFullscreen") == 0) {
			window_state.mode = WindowMode::eWindowedFullscreen;
			return true;
		} else if (mode.find("Windowed") == 0) {
			window_state.mode = WindowMode::eWindowed;
			return true;
		} else if (mode.find("Fullscreen") == 0) {
			window_state.mode = WindowMode::eFullscreen;
			return true;
		}
	}

	return false;
}

auto WindowState::FromFile(std::string_view file_path) -> std::optional<WindowState> {
	WindowState window_state;
	std::FILE*  config_file = std::fopen(file_path.data(), "r");
	if (!config_file) return std::nullopt;
	char line_buffer[256];
	// find [WindowState] line
	bool bFoundConfig = false;
	while (std::fgets(line_buffer, sizeof(line_buffer) - 1, config_file)) {
		std::string_view line{line_buffer, std::strlen(line_buffer)};
		if (line.find("[WindowState]") == 0) {
			bFoundConfig = true;
			break;
		}
	}
	if (!bFoundConfig) return std::nullopt;
	while (std::fgets(line_buffer, sizeof(line_buffer) - 1, config_file)) {
		std::string_view      line{line_buffer, std::strlen(line_buffer)};
		[[maybe_unused]] bool bParsed = ParseLine(window_state, line);
	};
	std::fclose(config_file);
	return window_state;
}
