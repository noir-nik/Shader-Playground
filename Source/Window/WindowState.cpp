module WindowState;
import std;
import IWindow;
import Window;
import ParseUtils;

auto WindowState::FromWindow(Window const& window) -> WindowState {
	int x, y, width, height;
	window.GetRestoredRect(x, y, width, height);
	return WindowState{x, y, width, height, window.GetWindowMode()};
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
	if (Utils::ParseInt(line, "x=", window_state.x) ||
		Utils::ParseInt(line, "y=", window_state.y) ||
		Utils::ParseInt(line, "width=", window_state.width) ||
		Utils::ParseInt(line, "height=", window_state.height)) {
		return true;
	};

	std::string_view mode;
	if (Utils::ParseString(line, "mode=", mode)) {
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
