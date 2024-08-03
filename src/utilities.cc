#include "utilities.h"

std::time_t now() {
    return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

std::chrono::_V2::steady_clock::time_point now_steady() {
    return std::chrono::steady_clock::now();
}



int64_t duration_ms(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
    if (end < start) {
        auto temp = start;
        start = end;
        end = temp;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

int64_t duration_s(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
    return duration_ms(start, end) / 1000;
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

std::time_t time_t_from_direntry(std::filesystem::directory_entry dir_entry) {
    auto file_time = std::filesystem::last_write_time(dir_entry);
    // Convert to system clock time point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        file_time - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now());
    // Convert to time_t
    return std::chrono::system_clock::to_time_t(sctp);
}