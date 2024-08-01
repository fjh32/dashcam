#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <filesystem>


std::time_t now();
std::string formatted_time();
std::string format_time(const char * time_format);
std::time_t time_t_from_direntry(std::filesystem::directory_entry dir_entry);