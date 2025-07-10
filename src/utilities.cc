#include "utilities.h"

void debugPrint(std::string message) {
    #ifdef DEBUG
    std::cout << message << std::endl;
    #endif
}

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

// uptime in seconds
std::time_t uptime() {
    // std::ifstream file = std::ifstream("/proc/uptime");
    std::ifstream file("/proc/uptime");
    std::string line;
    file >> line;
    return (std::time_t)std::stoul(line.substr(0, line.find(' ')));
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

std::time_t time_t_from_direntry1(std::filesystem::directory_entry dir_entry) {
    auto file_time = std::filesystem::last_write_time(dir_entry);
    // Convert to system clock time point
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        file_time - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now());
    // Convert to time_t
    return std::chrono::system_clock::to_time_t(sctp);
}

std::time_t time_t_from_direntry(std::filesystem::directory_entry dir_entry) {
    std::string filename = dir_entry.path().filename().string();

    // Extract timestamp from filename
    std::regex re(R"(output_(\d{2})-(\d{2})-(\d{4})_(\d{2})(\d{2})(\d{2}))");
    std::smatch match;

    if (std::regex_search(filename, match, re)) {
        std::tm tm{};
        tm.tm_mon = std::stoi(match[1]) - 1; // months: 0-11
        tm.tm_mday = std::stoi(match[2]);
        tm.tm_year = std::stoi(match[3]) - 1900;
        tm.tm_hour = std::stoi(match[4]);
        tm.tm_min = std::stoi(match[5]);
        tm.tm_sec = std::stoi(match[6]);
        tm.tm_isdst = -1;

        return std::mktime(&tm);
    }

    // Fallback: return mtime
    return std::chrono::system_clock::to_time_t(
        std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            std::filesystem::last_write_time(dir_entry) - std::filesystem::file_time_type::clock::now()
            + std::chrono::system_clock::now()));
}


void makeDir(const char * dir) {
    if (!std::filesystem::exists(dir)) {
        if (!std::filesystem::create_directory(dir)) {
            std::cerr << "Error: Could not create recording directory." << std::endl;
            exit(1); // handle better
        }
    }
}

void get_website_root() {
    // #ifdef DEBUG
    // std::cout << "Website root: " << WEBSITE_ROOT << std::endl;
    // #endif
    // #ifdef RELEASE
    // std::cout << "Website root: " << WEBSITE_ROOT << std::endl; 
    // #endif
}

std::string get_ip_address() {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *tempAddr = nullptr;

    // Retrieve the current interfaces - returns 0 on success
    if (getifaddrs(&interfaces) == 0) {
        // Loop through the list of interfaces
        tempAddr = interfaces;
        while (tempAddr != nullptr) {
            if (tempAddr->ifa_addr->sa_family == AF_INET) { // Check for IPv4 address
                // Get the IP address
                char ip[INET_ADDRSTRLEN];
                void* addr = &((struct sockaddr_in*)tempAddr->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, addr, ip, INET_ADDRSTRLEN);

                // Avoid loopback addresses
                if (strcmp(tempAddr->ifa_name, "lo") != 0) {
                    freeifaddrs(interfaces);
                    return ip;
                }
            }
            tempAddr = tempAddr->ifa_next;
        }
    }
    return "10.0.0.1";
}

std::vector<std::filesystem::directory_entry> getDirContents(std::string dir) {
    std::vector<std::filesystem::directory_entry> dir_contents;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (std::filesystem::is_regular_file(entry)) {
            dir_contents.push_back(entry);
        }
    }
    return dir_contents;
}

std::time_t file_time_to_time_t(std::filesystem::file_time_type ftime) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now()
        + system_clock::now()
    );
    return system_clock::to_time_t(sctp);
}