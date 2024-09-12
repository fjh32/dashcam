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
        configureCORS(drogon::app());
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
            return acc();
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

        drogon::app().registerHandler("/livestream.m3u8", 
                                        [this](const drogon::HttpRequestPtr& req,
                                        std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newFileResponse(this->directoryPath_ + "/livestream.m3u8");
            resp->addHeader("Content-Type", "application/vnd.apple.mpegurl");
            // resp->addHeader("Content-Type", "audio/mpegurl");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        });

        // Handler for .ts files
        drogon::app().registerHandler("/segment{}.ts", 
                                        [this](const drogon::HttpRequestPtr& req,
                                        std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                        const std::string &segment) {
            auto resp = drogon::HttpResponse::newFileResponse(this->directoryPath_ + "/segment" + segment + ".ts");
            resp->addHeader("Content-Type", "video/mp2t");
            // resp->addHeader("Content-Type", "application/octet-stream");
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
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



void HttpServer::configureCORS(drogon::HttpAppFramework& app) {
    app.registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr&,
           const drogon::HttpResponsePtr& resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Origin, Content-Type, Accept");
            resp->addHeader("Access-Control-Max-Age", "3600");
        }
    );

    // Handle OPTIONS requests for CORS preflight
    app.registerHandler(
        "/your/hls/path/.*",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            if (req->getMethod() == drogon::HttpMethod::Options) {
                resp->setStatusCode(drogon::HttpStatusCode::k204NoContent);
            }
            callback(resp);
        },
        {drogon::Get, drogon::Options}
    );
}