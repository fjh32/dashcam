#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <fstream>  
#include <filesystem>

#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>

#include <regex>


std::time_t now();
std::chrono::_V2::steady_clock::time_point now_steady();
std::time_t uptime();
int64_t duration_ms(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end);
int64_t duration_s(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end);
std::string formatted_time();
std::string format_time(const char * time_format);

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

std::string get_ip_address();

std::vector<std::filesystem::directory_entry> getDirContents(std::string);

std::time_t file_time_to_time_t(std::filesystem::file_time_type ftime) ;