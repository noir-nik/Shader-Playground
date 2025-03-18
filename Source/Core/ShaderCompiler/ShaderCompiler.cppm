export module ShaderCompiler;

import std;

export class ShaderCompiler {
public:
	~ShaderCompiler();
	bool               Init();
	void               Destroy();
	[[nodiscard]] bool CompileShader(std::string_view path, std::string_view output_file_path, std::string_view compile_options = "");
	auto               GetErrorMessage() -> std::string_view;

private:
	std::vector<std::byte> buffer;
	
};
