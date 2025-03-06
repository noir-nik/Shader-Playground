module;
#include <cstdio>
module ShaderCompiler;

import std;
import FileIOUtils;

static constexpr inline int kMaxCompileStringSize = 1024;

// static std::string gShaderCode;

struct FilePopener {
	FilePopener(char const* name, char const* mode) {
		file = popen(name, mode);
	}
	~FilePopener() {
		if (file) {
			pclose(file);
		}

		file = nullptr;
	}
	FilePopener(FilePopener const&)            = delete;
	FilePopener& operator=(FilePopener const&) = delete;
	FilePopener(FilePopener&& other) {
		if (file) {
			pclose(file);
		}
		file       = other.file;
		other.file = nullptr;
	}
	FilePopener& operator=(FilePopener&& other) {
		if (this != &other) {
			if (file) {
				pclose(file);
			}
			file       = other.file;
			other.file = nullptr;
		}
		return *this;
	}

	operator FILE*() const { return file; }
	FILE* file = nullptr;
};

bool ShaderCompiler::Init() { return true; };
ShaderCompiler::~ShaderCompiler() { Destroy(); }
void ShaderCompiler::Destroy() {};

// auto ShaderCompiler::LoadShader(std::string_view path) -> std::optional<std::string_view> {
auto ShaderCompiler::LoadShader(std::string_view path) -> std::optional<std::span<std::byte const>> {
	std::basic_string_view<char> const file_extension = path.substr(path.find_last_of('.') + 1);

	char const* compiler   = "";
	char const* entry_flag = "";
	char const* target     = "";
	char const* options    = "";
	char const* output_file    = "frag_output.spv";
	if (file_extension == "glsl" || file_extension == "frag") {
		compiler   = "glslc";
		entry_flag = "-fentry-point=";
	} else if (file_extension == "slang") {
		compiler   = "slangc";
		entry_flag = "-entry ";
		options    = "-target spirv";
	} else {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Unknown file extension: %s", file_extension.data());
		return std::nullopt;
	}

	char compile_string[kMaxCompileStringSize];

	std::snprintf(compile_string, kMaxCompileStringSize - 1, "%s %s %s -o %s %s%s %s", compiler, options, path.data(), output_file, entry_flag, "main", target);
	std::printf("Compiling: %s\n", compile_string);

	if(std::system(compile_string)) {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to compile shader: %s\n Compile string: %s", path.data(), compile_string);
		return std::nullopt;
	}

	std::ifstream file(output_file, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to open compiled shader: %s", output_file);
		return std::nullopt;
	}
	std::size_t            file_size = static_cast<std::size_t>(file.tellg());
	// std::vector<std::byte> buffer(file_size);
	shader_code.resize(file_size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(shader_code.data()), file_size);
	file.close();
	return shader_code;


	// FilePopener output(compile_string, "r");
	// if (!output) {
	// 	error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to compile shader: %s", path.data());
	// 	return std::nullopt;
	// }

	// shader_code.clear();
	// char        buffer[1024 * 8];
	// std::size_t total_size = 0;
	// while (!std::feof(output)) {
	// 	std::size_t size = std::fread(buffer, sizeof(buffer[0]), sizeof(buffer), output);
	// 	if (std::ferror(output)) {
	// 		error_size = std::snprintf(error_buffer, sizeof(error_buffer) - 1, "Failed to read shader compiler output: %s", path.data());
	// 		return std::nullopt;
	// 	}
	// 	// gShaderCode.insert(gShaderCode.end(), buffer, buffer + std::strlen(buffer));`
	// 	auto byte_buffer = reinterpret_cast<std::byte const*>(buffer);
	// 	shader_code.insert(shader_code.end(), byte_buffer, byte_buffer + size);
	// 	total_size += size;
	// }

	// std::fwrite(shader_code.data(), sizeof(shader_code[0]), shader_code.size(), stdout);
	// std::printf("Total size: %zu\n", total_size);

	// gShaderCode.resize(1024);
	// while (auto const bytes_read = fread(gShaderCode.data(), 1, gShaderCode.size(), output)) {
	// 	gShaderCode.resize(gShaderCode.size() + bytes_read);
	// }

	return shader_code;
};

auto ShaderCompiler::GetErrorMessage() -> std::string_view {
	return {error_buffer, error_size};
};
