#ifndef FPMAS_LOG_H
#define FPMAS_LOG_H

/** \file src/fpmas/utils/log.h
 * A logging library, inspired from the
 * [esp-idf](https://github.com/espressif/esp-idf/tree/master/components/log).
 */

#include <chrono>
#include <iostream>

typedef enum {
    FPMAS_LOG_NONE,       /*!< No log output */
    FPMAS_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    FPMAS_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    FPMAS_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    FPMAS_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    FPMAS_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} fpmas_log_level_t;

#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR_WHITE   "37"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D       LOG_COLOR(LOG_COLOR_PURPLE)
#define LOG_COLOR_V

#define LOG_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " %li [%i] %s : " format LOG_RESET_COLOR "\n"

#define FPMAS_LOGE( rank, tag, format, ... ) FPMAS_LOG_LEVEL(FPMAS_LOG_ERROR,   rank, tag, format, ##__VA_ARGS__)
#define FPMAS_LOGW( rank, tag, format, ... ) FPMAS_LOG_LEVEL(FPMAS_LOG_WARN,    rank, tag, format, ##__VA_ARGS__)
#define FPMAS_LOGI( rank, tag, format, ... ) FPMAS_LOG_LEVEL(FPMAS_LOG_INFO,    rank, tag, format, ##__VA_ARGS__)
#define FPMAS_LOGD( rank, tag, format, ... ) FPMAS_LOG_LEVEL(FPMAS_LOG_DEBUG,   rank, tag, format, ##__VA_ARGS__)
#define FPMAS_LOGV( rank, tag, format, ... ) FPMAS_LOG_LEVEL(FPMAS_LOG_VERBOSE, rank, tag, format, ##__VA_ARGS__)

#define FPMAS_LOG_LEVEL_IMPL(level, rank, tag, format, ...) do {                     \
        if (level==FPMAS_LOG_ERROR )          { fpmas_log_write(LOG_FORMAT(E, format), current_time(), rank, tag, ##__VA_ARGS__); } \
        else if (level==FPMAS_LOG_WARN )      { fpmas_log_write(LOG_FORMAT(W, format), current_time(), rank, tag, ##__VA_ARGS__); } \
        else if (level==FPMAS_LOG_DEBUG )     { fpmas_log_write(LOG_FORMAT(D, format), current_time(), rank, tag, ##__VA_ARGS__); } \
        else if (level==FPMAS_LOG_VERBOSE )   { fpmas_log_write(LOG_FORMAT(V, format), current_time(), rank, tag, ##__VA_ARGS__); } \
        else                                { fpmas_log_write(LOG_FORMAT(I, format), current_time(), rank, tag, ##__VA_ARGS__); } \
    } while(0)

#define FPMAS_LOG_LEVEL(level, rank, tag, format, ...) do {               \
        if ( LOG_LEVEL >= level ) FPMAS_LOG_LEVEL_IMPL(level, rank, tag, format, ##__VA_ARGS__); \
    } while(0)

static auto start = std::chrono::system_clock::now();

void fpmas_log_write(const char* format, ...);
long current_time();

#endif
