#include "HttpServer.h"

HttpServer::HttpServer(const std::string &directory, uint16_t port)
        : directoryPath_(directory), port_(port) {}

    // Function to start the web server in a new thread
void HttpServer::startHttpServer() {
    debugPrint("Starting HTTP server on port " + std::to_string(port_) + " serving directory " + directoryPath_);
    // Launch the server in a separate thread
    serverThread_ = std::thread([this]() {
        drogon::app().disableSigtermHandling();
        // Configure Drogon to serve the directory statically
        // drogon::app().registerFilter(std::make_shared<CookieChecker>());
        drogon::app().setDocumentRoot(directoryPath_);
        drogon::app().setFileTypes({"gif",
                                    "png",
                                    "jpg",
                                    "js",
                                    "css",
                                    "html",
                                    "ico",
                                    "swf",
                                    "xap",
                                    "apk",
                                    "cur",
                                    "m3u8",
                                    "ts",
                                    "mkv",
                                    "mp4"
                                        });
        // Set the server's port
        drogon::app().addListener("0.0.0.0", port_);
        // Run the web server
        drogon::app().registerPreRoutingAdvice([](const HttpRequestPtr &req, 
                                                        AdviceCallback &&ac, 
                                                        AdviceChainCallback &&acc) {
            const auto& cookie = req->getCookie("coconut");
            if (cookie.empty()) {
                // If the cookie is not present, return a 403 Forbidden response
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                resp->setBody("Access denied: Missing required cookie");
                return ac(resp);
            }
            else {
                // Cookie is present, continue to the next filter or handler
                return acc();
            }
        });

        
        drogon::app().run();
    });
    serverThread_.detach();
}

HttpServer::~HttpServer() {
    // Stop the Drogon application gracefully
    drogon::app().quit();
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    debugPrint("HTTP server stopped");
}
