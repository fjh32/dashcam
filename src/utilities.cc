#include "utilities.h"

std::time_t now() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::string formatted_time() {
    return format_time("%m-%d-%Y_%H%M%S");
}

std::string format_time(const char * time_format) {
    std::ostringstream ss;
    long curtime = std::time(nullptr);
    tm * tmptr = std::localtime(&curtime);
    ss << std::put_time(tmptr, time_format);
    return ss.str();
}