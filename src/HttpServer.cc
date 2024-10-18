#include "HttpServer.h"

HttpServer::HttpServer(const std::string &recordingsPath, uint16_t port)
        : recordingsPath_(recordingsPath), web_static_dir_(recordingsPath), port_(port) {}

HttpServer::HttpServer(const std::string &recordingsPath, const std::string &web_static_dir, uint16_t port)
        : recordingsPath_(recordingsPath), web_static_dir_(web_static_dir), port_(port) {}

    // Function to start the web server in a new thread
void HttpServer::startHttpServer() {
    debugPrint("Starting HTTP server on port " + std::to_string(port_));
    debugPrint("Serving files from " + web_static_dir_);
    debugPrint("Recordings dir " + recordingsPath_);
    // Launch the server in a separate thread
    serverThread_ = std::thread(&HttpServer::runServer, this);
    serverThread_.detach();
}

void HttpServer::runServer() {
    drogon::app().disableSigtermHandling();
    drogon::app().setDocumentRoot(web_static_dir_);
    drogon::app().addListener("0.0.0.0", port_);
    // register_middleware();
    register_handlers();
    drogon::app().run();
}

void HttpServer::register_middleware() {
    // Check for the presence of a cookie
    drogon::app().registerPreRoutingAdvice([](const HttpRequestPtr &req, 
                                                    AdviceCallback &&ac, 
                                                    AdviceChainCallback &&acc) {
        const auto& cookie = req->getCookie("coconut");
        // #ifdef DEBUG
        // acc();
        // #endif
        // parse cookie here
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
}

void HttpServer::register_handlers() {
    drogon::app().registerHandler("/livestream.m3u8",
                                    [this](const drogon::HttpRequestPtr& req,
                                    std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        auto resp = drogon::HttpResponse::newFileResponse(this->recordingsPath_ + "/livestream.m3u8");
        resp->addHeader("Content-Type", "application/vnd.apple.mpegurl");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    });

    // Handler for .ts files
    drogon::app().registerHandler("/segment{}.ts", 
                                    [this](const drogon::HttpRequestPtr& req,
                                    std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                    const std::string &segment) {
        auto resp = drogon::HttpResponse::newFileResponse(this->recordingsPath_ + "/segment" + segment + ".ts");
        resp->addHeader("Content-Type", "video/mp2t");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    });

    drogon::app().registerHandler("/recordings/{filename}",
                                    [this](const drogon::HttpRequestPtr& req,
                                    std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                    const std::string &filename) {
        auto resp = drogon::HttpResponse::newFileResponse(this->recordingsPath_ + "/" + filename);
        resp->addHeader("Access-Control-Allow-Origin", "*");
        callback(resp);
    });

    drogon::app().registerHandler("/shutdown_cam_service",
                                    [this](const drogon::HttpRequestPtr& req,
                                    std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        // Shutdown the CamService;
        //  /tmp/camrecorder.pipe
        std::ofstream pipeFile("/tmp/camrecorder.pipe");
        std::cout << "HTTP REQUEST ON /shutdown_cam_service. Sending kill message to CamService pipe." << std::endl;
        pipeFile << "kill" << std::endl;
        pipeFile.close();
        callback(drogon::HttpResponse::newHttpResponse());
    }); 
}

HttpServer::~HttpServer() {
    // Stop the Drogon application gracefully
    drogon::app().getLoop()->runInLoop([]() {
        drogon::app().quit();  // This will stop Drogon
    });
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    debugPrint("HTTP server stopped");
}