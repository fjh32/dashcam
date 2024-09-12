#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <filesystem>


std::time_t now();
std::chrono::_V2::steady_clock::time_point now_steady();
int64_t duration_ms(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end);
int64_t duration_s(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end);
std::string formatted_time();
std::string format_time(const char * time_format);
std::time_t time_t_from_direntry(std::filesystem::directory_entry dir_entry);

void debugPrint(std::string message);

template <typename T,typename L = std::chrono::_V2::steady_clock::time_point>
int64_t duration(L start, L end) {
    if(end < start) {
        auto temp = start;
        start = end;
        end = temp;
    }
    return std::chrono::duration_cast<T>(end - start).count();
}

void makeDir(const char * dir);

void get_website_root();