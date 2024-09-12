#pragma once
#include <drogon/drogon.h>
#include <thread>
#include <memory>
#include "utilities.h"
#include <drogon/utils/Utilities.h>

using namespace drogon;

class HttpServer {
    public:
        HttpServer(const std::string &directory, uint16_t port);
        void startHttpServer();
        ~HttpServer();
        void configureCORS(drogon::HttpAppFramework& app);

    private:
        std::string directoryPath_; // Directory to be served
        uint16_t port_;             // Port number to listen on
        std::thread serverThread_;  // Thread to run the Drogon application
};

