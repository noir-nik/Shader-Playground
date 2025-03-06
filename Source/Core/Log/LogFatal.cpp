#include <cstdio> // stdout, stderr
#include <cstdarg> // va_start, va_end
#include "LogConstants.hpp"
#include "LogMacros.hpp"
import Log;
import std;

void HandleLogFatalError(char const* file, int line, char const* fmt, ...) {
	std::va_list args;
	va_start(args, fmt);
	char buffer[kMessageBufferSize];
	int size = 0;
	// size += snprintf(buffer + size, sizeof(buffer) - size - 1, "\nError message: ");
	size += vsnprintf(buffer + size, sizeof(buffer) - size - 1, fmt, args);
	size += snprintf(buffer + size, sizeof(buffer) - size - 1, "\nError occurred in %s:%d ", file, line);
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	// todo: Messagebox
	LOG_MESSAGE(LogLevel::Fatal, buffer);
	std::exit(1);
}
