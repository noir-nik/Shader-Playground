module;
module FileManager;

import std;
import FileIOUtils;

static constexpr inline int kMaxCompileStringSize = 1024;

auto FileManager::ReadFile(std::string_view const filename) -> std::optional<std::string> {
	if (auto file = std::ifstream(filename.data(), std::ios::ate)) {
		std::string content;
		content.reserve(static_cast<std::size_t>(file.tellg()));
		file.seekg(0);
		content.assign((std::istreambuf_iterator<std::string::value_type>(file)),
					   std::istreambuf_iterator<std::string::value_type>());
		file.close();
		return std::move(content);
	}
	return std::nullopt;
}

auto FileManager::ReadBinaryFile(std::string_view const filename) -> std::optional<std::span<std::byte>> {
	std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return std::nullopt;
	}
	std::size_t file_size = static_cast<std::size_t>(file.tellg());
	file_buffer.resize(file_size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(file_buffer.data()), file_size);
	file.close();
	return file_buffer;
}

auto FileManager::ReadBinaryFileUnique(std::string_view const filename) -> std::optional<std::vector<std::byte>> {
	std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return std::nullopt;
	}
	std::size_t            file_size = static_cast<std::size_t>(file.tellg());
	std::vector<std::byte> buffer(file_size);
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), file_size);
	file.close();
	return std::move(buffer);
}

auto FileManager::GetFileVersion(std::string_view const filename) -> int {
	std::filesystem::path file_path(filename);
	if (!std::filesystem::exists(file_path)) {
		return -1;
	}
	auto last_write_time = std::filesystem::last_write_time(file_path);
	auto duration        = last_write_time.time_since_epoch();
	auto seconds         = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	return static_cast<int>(seconds);
}

auto FileManager::GetErrorMessage() -> std::string_view {
	return {error_buffer, error_size};
};
