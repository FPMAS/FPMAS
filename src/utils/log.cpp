#include <cstdio>
#include <cstdarg>

#include "log.h"

void fpmas_log_write(const char *format, ...)
{
	std::va_list list;
	va_start(list, format);
	std::vprintf(format, list);
    va_end(list);
}

long current_time() {
	auto now = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}
