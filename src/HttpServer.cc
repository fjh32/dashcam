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
        std::cout << "HTTP REQUEST ON /shutdown_cam_service. Sending kill message to CamService pipe in 1 second." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pipeFile << "kill" << std::endl;
        pipeFile.close();
        callback(drogon::HttpResponse::newHttpResponse());
    });

    drogon::app().registerHandler("/recording_list",
    [this](const drogon::HttpRequestPtr& req,
    std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
        // std::string response = "<html><head><title>Recordings</title></head><body><h1>Recordings</h1><ul>";
        std::vector<std::filesystem::directory_entry> dir_contents = getDirContents(this->recordingsPath_);
        dir_contents.erase(std::remove_if(dir_contents.begin(), dir_contents.end(), [](const std::filesystem::directory_entry &entry) {
            std::string filename = entry.path().filename().string();
            return (filename.find(".mp4") == std::string::npos);
	      //                || (filename.find("output") != std::string::npos);
        }), dir_contents.end());
        
        std::ranges::sort(dir_contents, [](const std::filesystem::directory_entry &a, const std::filesystem::directory_entry &b) {
            return a.last_write_time() > b.last_write_time();
        });
        // dir_contents.erase(dir_contents.begin()); // delete the newest element, it is 
        
        std::vector<std::string> dir_contents_str;
        for(auto &entry : dir_contents) {
            dir_contents_str.push_back(entry.path().filename().string());
        }
        auto resp = json_response_from_vector(dir_contents_str);
        callback(resp);
    });

    drogon::app().registerHandler("/save_recording", 
        [this](const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
            auto params = req->getParameters();
            // auto recordingName = req->getParameter("recording_name");
            std::time_t uptime_sec = uptime();
            std::string saveOnPipeMsg = "save:" + std::to_string(uptime_sec); // + :<recordingname> // add support for this
            std::cout << "Saving recording: " << saveOnPipeMsg << std::endl;
            std::ofstream pipeFile("/tmp/camrecorder.pipe");
            pipeFile << saveOnPipeMsg << std::endl;
            pipeFile.close();
            auto resp = drogon::HttpResponse::newRedirectionResponse("/");
            callback(resp);
        }
    ,{drogon::Post});
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
