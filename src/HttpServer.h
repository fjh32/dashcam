#pragma once
#include <drogon/drogon.h>
#include <thread>
#include <memory>
#include "utilities.h"
#include <drogon/utils/Utilities.h>
#include <iostream>
#include <fstream>

using namespace drogon;

class HttpServer {
    public:
        HttpServer(const std::string &directory, uint16_t port);
        HttpServer(const std::string &recordingsPath, const std::string &web_static_dir, uint16_t port);
        void startHttpServer();
        ~HttpServer();
    private:
        void runServer();
        void register_middleware();
        void register_handlers();
        std::string recordingsPath_; // Directory to be served
        std::string web_static_dir_;
        uint16_t port_;             // Port number to listen on
        std::thread serverThread_;  // Thread to run the Drogon application
};

