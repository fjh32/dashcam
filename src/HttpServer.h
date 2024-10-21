#pragma once
#include <drogon/drogon.h>
#include <thread>
#include <memory>
#include "utilities.h"
#include <drogon/utils/Utilities.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <ranges>

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

template <typename T>
std::shared_ptr<HttpResponse> json_response_from_vector(std::vector<T> data) {
    
    Json::Value jsonArray(Json::arrayValue);
    for(const auto &item : data) {
        jsonArray.append(item);
    }

    auto resp = HttpResponse::newHttpJsonResponse(jsonArray);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    return resp;
}