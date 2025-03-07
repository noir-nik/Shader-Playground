module ApplicationPlatform;
import std;

auto GetConfigPath(char const app_name[]) -> std::string {
	std::string config_path;
	config_path.reserve(260);

#ifdef _WIN32
	const char* appdata = std::getenv("APPDATA");
	if (appdata) {
		config_path.append(appdata).append("\\").append(app_name);
	} else {
		config_path = ".";
	}
#elif __linux__
	const char* home = std::getenv("HOME");
	if (home) {
		config_path.append(home).append("/.config/").append(app_name);
	} else {
		config_path = ".";
	}
#elif __APPLE__
	const char* home = std::getenv("HOME");
	if (home) {
		config_path.append(home).append("/Library/Preferences/").append(app_name);
	} else {
		config_path = ".";
	}
#else
	config_path = ".";
#endif
	return config_path;
}

auto GetTempDirectory() -> char const* {
	char const* path;
    #ifdef _WIN32
        path = std::getenv("TMP");
		if (!path) path = std::getenv("TEMP");
		if (!path) path = "C:\\Windows\\Temp";
    #elif __linux__ || __APPLE__
        const char* path = getenv("TMPDIR");
        if (!path) path = "/tmp";
    #else
        path = "./tmp";
    #endif
    return path;
}