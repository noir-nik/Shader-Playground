export module ShaderCompiler;

import std;

export class ShaderCompiler {
public:
	~ShaderCompiler();
	bool Init();
	void Destroy();
	// [[nodiscard]] auto LoadShader(std::string_view path) -> std::optional<std::string_view>;
	[[nodiscard]] auto LoadShader(std::string_view path) -> std::optional<std::span<std::byte const>>;
	auto               GetErrorMessage() -> std::string_view;

private:
	std::vector<std::byte> shader_code;
	char                   error_buffer[1024];
	std::uint32_t          error_size = 0;
};
