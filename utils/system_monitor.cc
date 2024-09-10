#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <sstream>
#include <thread>

using namespace std;
namespace fs = std::filesystem;

long timestamp() {
    return std::time(nullptr);
}

std::string format_time(const char * time_format) {
    std::ostringstream ss;
    long curtime = std::time(nullptr);
    tm * tmptr = std::localtime(&curtime);
    ss << std::put_time(tmptr, time_format);
    return ss.str();
}

std::string formatted_time() {
    return format_time("%m-%d-%Y %H:%M:%S");
}

double calculate_idle_cpu_time() {
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
    std::istringstream ss(line);
    std::string cpustring;
    ss >> cpustring; // ignore cpu string
    if(cpustring.find("cpu") == std::string::npos) { // check if the first word is cpu
        return -1.0;
    }
    int user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    ss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    double idle_time = idle + iowait;
    double non_idle_time = user + nice + system + irq + softirq + steal;
    double total_time = idle_time + non_idle_time;
    return idle_time * 100 / total_time;
}

double calculate_used_cpu() {
    return 100 - calculate_idle_cpu_time();
}

int mem_usage() {
    std::ifstream file("/proc/meminfo");
    int total, free, available, buffers, cached;
    std::string line;
    while(std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key;
        ss >> key;
        if(key == "MemTotal:") {
            ss >> total;
        } else if(key == "MemFree:") {
            ss >> free;
        } else if(key == "MemAvailable:") {
            ss >> available;
        } else if(key == "Buffers:") {
            ss >> buffers;
        } else if(key == "Cached:") {
            ss >> cached;
        }
    }
    return (total - free - buffers - cached) / 1000; // MB
}

bool checkProcessRunning(string procname) {
    string command = "ps aux | grep " + procname + " | grep -v grep";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "popen() failed!" << std::endl;
        return false;
    }

    stringstream ss;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        ss << buffer;
    }
    pclose(pipe);
    return ss.str().length() > 0;
}

double temperature() {
    double temp;
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    file >> temp;
    return temp / 1000; 
}

string writeCsvLine(string procname) {
    string procRunning = checkProcessRunning(procname) ? "true" : "false";
    return to_string(timestamp()) + "," 
            + to_string(calculate_used_cpu()) + "," 
            + to_string(mem_usage()) + ","
            + to_string(temperature()) + ","
            + procRunning;
}

int main() {
    std::ofstream file("dashcam_sys_monitor.csv", std::ios_base::app);
    while(true) {
        file << writeCsvLine("dashcam") << endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    file.close();
    return 0;
}