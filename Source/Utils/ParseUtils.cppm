export module ParseUtils;
import std;
export namespace Utils {

auto ParseInt(std::string_view const line, std::string_view const key, int& value) -> bool {
	if (std::size_t pos = line.find(key); pos == 0) {
		std::size_t num_read = std::sscanf(line.data() + pos + std::size(key), "%d", &value);
		if (num_read == 1) return true;
	}
	return false;
};

auto ParseString(std::string_view const line, std::string_view const key, std::string_view& value) -> bool {
	if (std::size_t pos = line.find(key); pos == 0) {
		value = line.substr(pos + std::size(key));
		return true;
	}
	return false;
};
} // namespace Utils
