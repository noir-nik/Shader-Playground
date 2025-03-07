export module ApplicationGlobalData;
import std;

export struct ApplicationGlobalData {
	std::filesystem::path executable_path;
	std::string           executable_path_string;
	std::filesystem::path executable_dir;
	std::string           executable_dir_string;
	std::string_view      application_title = "Shader Playground";
	std::string           config_dir        = ".";
	std::filesystem::path temp_dir;
	std::string           temp_dir_string;
	std::string_view      fallback_fragment_shader_file_path = "Source/Shaders/Fallback.frag";
	std::string           fallback_fragment_spv_path;
	std::string           user_fragment_spv_path;
	std::string           window_state_path;
};

export extern ApplicationGlobalData gGlobalData;
