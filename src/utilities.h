#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>


std::time_t now();
std::string formatted_time();
std::string format_time(const char * time_format);