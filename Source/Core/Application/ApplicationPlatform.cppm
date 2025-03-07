export module ApplicationPlatform;
import std;

auto GetConfigPath(char const app_name[]) -> std::string;
auto GetTempDirectory() -> char const*;
