export module WindowState;
import Window;
import std;

export struct WindowState {
	[[nodiscard]] static auto FromFile(std::string_view file_path) -> std::optional<WindowState>;
	[[nodiscard]] static auto FromWindow(Window const& window) -> WindowState;

	bool SaveToFile(std::string_view file_path);

	int        x      = kWindowDontCare;
	int        y      = kWindowDontCare;
	int        width  = kWindowDontCare;
	int        height = kWindowDontCare;
	WindowMode mode   = WindowMode::eMaxEnum;
};
