module;
// #include <cassert> // assert
#include <cstdarg> // va_start, va_end
module ShaderCompiler;

import std;
import FileIOUtils;

bool ShaderCompiler::Init() { return true; };
ShaderCompiler::~ShaderCompiler() { Destroy(); }
void ShaderCompiler::Destroy() { buffer.clear(); }

int FormatAndResize(std::vector<std::byte>& buffer, char const* format, ...) {
	va_list args;
	va_start(args, format);
	int size = std::vsnprintf(reinterpret_cast<char*>(buffer.data()), buffer.size(), format, args) + 1;
	va_end(args);
	std::size_t old_size = buffer.size();
	buffer.resize(size);
	if (size > old_size) {
		va_start(args, format);
		int new_size = std::vsnprintf(reinterpret_cast<char*>(buffer.data()), buffer.size(), format, args) + 1;
		va_end(args);
		// assert(new_size == buffer.size());
	}
	return size;
}

bool ShaderCompiler::CompileShader(std::string_view path, std::string_view output_file_path, std::string_view compile_options) {
	std::basic_string_view<char> const file_extension = path.substr(path.find_last_of('.') + 1);

	char const* user_options     = compile_options.empty() ? "" : compile_options.data();
	char const* compiler         = "";
	char const* entry_flag       = "";
	char const* target           = "";
	char const* compiler_options = "";
	if (file_extension == "glsl" || file_extension == "frag") {
		compiler   = "glslc";
		entry_flag = "-fentry-point=";
	} else if (file_extension == "slang") {
		compiler         = "slangc";
		entry_flag       = "-entry ";
		compiler_options = "-target spirv";
	} else {
		FormatAndResize(buffer, "Unknown file extension: %s", file_extension.data());
		return false;
	}

	FormatAndResize(buffer, "%s %s %s -o %s %s%s %s %s",
					compiler, compiler_options, path.data(),
					output_file_path.data(), entry_flag, "main", target, user_options);

	if (std::system(reinterpret_cast<char*>(buffer.data()))) {
		return false;
	}
	return true;
}

auto ShaderCompiler::GetErrorMessage() -> std::string_view {
	return {reinterpret_cast<char*>(buffer.data()), buffer.size()};
};
