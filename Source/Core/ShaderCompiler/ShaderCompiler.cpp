module;
#include <cstdio>
module ShaderCompiler;

import std;
import FileIOUtils;

static constexpr inline int kMaxCompileStringSize = 1024;

bool ShaderCompiler::Init() { return true; };
ShaderCompiler::~ShaderCompiler() { Destroy(); }
void ShaderCompiler::Destroy() {};

bool ShaderCompiler::CompileShader(std::string_view path, std::string_view output_file_path) {
	std::basic_string_view<char> const file_extension = path.substr(path.find_last_of('.') + 1);

	char const* compiler   = "";
	char const* entry_flag = "";
	char const* target     = "";
	char const* options    = "";
	if (file_extension == "glsl" || file_extension == "frag") {
		compiler   = "glslc";
		entry_flag = "-fentry-point=";
	} else if (file_extension == "slang") {
		compiler   = "slangc";
		entry_flag = "-entry ";
		options    = "-target spirv";
	} else {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Unknown file extension: %s", file_extension.data());
		return false;
	}

	char compile_string[kMaxCompileStringSize];

	std::snprintf(compile_string, kMaxCompileStringSize - 1, "%s %s %s -o %s %s%s %s", compiler, options, path.data(), output_file_path.data(), entry_flag, "main", target);
	// std::printf("Compiling: %s\n", compile_string);

	if (std::system(compile_string)) {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to compile shader: %s\nCompile string: %s", path.data(), compile_string);
		return false;
	}
	return true;
}

auto ShaderCompiler::LoadShader(std::string_view path, std::string_view output_file_path) -> std::optional<std::span<std::byte const>> {
	char const* output_file = output_file_path.data();

	if (!CompileShader(path, output_file)) {
		return std::nullopt;
	}

	std::ifstream file(output_file, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to open compiled shader: %s", output_file);
		return std::nullopt;
	}
	std::size_t file_size = static_cast<std::size_t>(file.tellg());
	shader_code.resize(file_size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(shader_code.data()), file_size);
	file.close();
	return shader_code;
};

auto ShaderCompiler::GetErrorMessage() -> std::string_view {
	return {error_buffer, error_size};
};
