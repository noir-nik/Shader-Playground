export module FileManager;

import std;

export class FileManager {
public:
	auto GetErrorMessage() -> std::string_view;

	[[nodiscard]] auto ReadFile(std::string_view const filename) -> std::optional<std::string>;
	[[nodiscard]] auto ReadBinaryFile(std::string_view const filename) -> std::optional<std::span<std::byte>>;
	[[nodiscard]] auto ReadBinaryFileUnique(std::string_view const filename) -> std::optional<std::vector<std::byte>>;
	[[nodiscard]] auto GetFileVersion(std::string_view const filename) -> int;

private:
	std::vector<std::byte> file_buffer;
	char                   error_buffer[1024];
	std::uint32_t          error_size = 0;
};
