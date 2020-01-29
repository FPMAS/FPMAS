#include <cstdio>
#include <cstdarg>

#include "log.h"

void fpmas_log_write(const char *format, ...)
{
	//std::cout << "f:" << format << std::endl;
	std::va_list list;
	va_start(list, format);
	std::vprintf(format, list);
    va_end(list);
}

long current_time() {
	auto now = std::chrono::system_clock::now();
	/*
	 *std::time_t time = std::chrono::system_clock::to_time_t(now);
	 *std::strftime(
	 *        current_time_buffer,
	 *        18,
	 *        "%T",
	 *        std::localtime(&time)
	 *        );
	 */
	return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}
