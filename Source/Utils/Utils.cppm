export module Utils;
export import FileIOUtils;
export import VulkanUtils;
export import ParseUtils;

import std;
export namespace Utils {
constexpr inline auto FormatBool(bool value) -> std::string_view { return value ? "true" : "false"; };
} // namespace Utils
