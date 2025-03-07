export module FileIOUtils;
import std;

export namespace Utils {
[[nodiscard]] auto ReadFile(std::string_view const filename) -> std::optional<std::string>;
[[nodiscard]] auto ReadBinaryFile(std::string_view const filename) -> std::optional<std::vector<std::byte>>;
[[nodiscard]] auto GetFileVersion(std::string_view const filename) -> int;
} // namespace Utils
