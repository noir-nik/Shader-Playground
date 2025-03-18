export module Utils;
export import FileIOUtils;
export import VulkanUtils;
export import ParseUtils;

import std;
export namespace Utils {
constexpr inline auto FormatBool(bool value) -> std::string_view { return value ? "true" : "false"; };

inline void HashCombine(std::size_t& hash, std::size_t value) {
	hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}
} // namespace Utils
