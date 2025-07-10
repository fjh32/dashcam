#include "CamService.h"

CamService::CamService(int* argc, char** argv[]) {
    debugPrint("Creating CamService object");
    recordingDir = RECORDING_DIR;
    recordingSaveDir = RECORDING_SAVE_DIR;
    prepDirForService();
    createListeningPipe();

    #ifdef RPI_MODE
    std::cout << "RPI MODE CamService" << std::endl;
    recordingPipeline = make_unique<RecordingPipeline>(RECORDING_DIR, VIDEO_DURATION, argc, argv);
    recordingPipeline->setSource(std::make_unique<LibcameraPipelineSource>());
    recordingPipeline->addSink(std::make_unique<TsFilePipelineSink>(recordingPipeline.get(), false, SEGMENTS_TO_KEEP));
    recordingPipeline->addSink(std::make_unique<HlsPipelineSink>(recordingPipeline.get()));
    #elif defined(EXPERIMENTAL_MODE)
    std::cout << "EXPERIMENTAL_MODE CamService" << std::endl;
    recordingPipeline = make_unique<RecordingPipeline>(RECORDING_DIR, 2, argc, argv);
    recordingPipeline->setSource(std::make_unique<V4l2PipelineSource>());
    recordingPipeline->addSink(std::make_unique<TsFilePipelineSink>(recordingPipeline.get(), false,10));
    recordingPipeline->addSink(std::make_unique<HlsPipelineSink>(recordingPipeline.get()));
    #else
    std::cout << "V4L2 MODE CamService" << std::endl;
    recordingPipeline = make_unique<RecordingPipeline>(RECORDING_DIR, VIDEO_DURATION, argc, argv);
    recordingPipeline->setSource(std::make_unique<V4l2PipelineSource>());
    recordingPipeline->addSink(std::make_unique<TsFilePipelineSink>(recordingPipeline.get(), false, SEGMENTS_TO_KEEP));
    recordingPipeline->addSink(std::make_unique<HlsPipelineSink>(recordingPipeline.get()));
    #endif

    running = false;
}

CamService::~CamService() {
    debugPrint("Destroying CamService object");
}

void CamService::mainLoop() {
    cout << "Starting CamService::mainLoop() at " << formatted_time() << endl;
    running = true;
    recordingPipeline->startPipeline(); // non-blocking call
    // cleanupThread = std::thread(&CamService::cleanupThreadLoop, this);
    listenOnPipe();
    cout << "Exiting main loop" << endl;
}

void CamService::killMainLoop() {
    running = false;
    recordingPipeline->stopPipeline();
    
    // cleanupThread.join();
    removeListeningPipe();
    cout << "Killed CamService at " << formatted_time() << endl;
}

/// private functions ///////////////////////////////////////////
bool CamService::isRecording() {
    return recordingPipeline->pipelineRunning;
}

void CamService::saveRecordings(int seconds_back_to_save) {
    auto current_time = now();
    auto threshold_time = current_time - seconds_back_to_save;

    std::regex segment_pattern(R"(output_(\d+)\.ts)");
    std::regex subdir_pattern(R"(^\d+$)");

    std::vector<std::filesystem::directory_entry> candidates;

    for (const auto& dir_entry : std::filesystem::directory_iterator(recordingDir)) {
        if (!dir_entry.is_directory()) continue;

        std::string subdir_name = dir_entry.path().filename().string();
        if (!std::regex_match(subdir_name, subdir_pattern)) continue;

        for (const auto& file_entry : std::filesystem::directory_iterator(dir_entry)) {
            if (!file_entry.is_regular_file()) continue;

            const std::string filename = file_entry.path().filename().string();
            if (!std::regex_match(filename, segment_pattern)) continue;

            std::time_t mtime = file_time_to_time_t(std::filesystem::last_write_time(file_entry));
            if (mtime > threshold_time) {
                candidates.push_back(file_entry);
            }
        }
    }

    this->recordingPipeline->createNewVideo();

    std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
        return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
    });

    std::string timestamp_dir = recordingSaveDir + std::to_string(current_time) + "/";
    std::filesystem::create_directories(timestamp_dir); // Ensure directory exists
    std::cout << "Saving " << candidates.size() << " recent segments to " << timestamp_dir << std::endl;
    for (const auto& entry : candidates) {
        std::string src = entry.path().string();
        std::string dst = timestamp_dir + entry.path().filename().string();

        try {
            std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
            std::cout << "Saved file: " << dst << std::endl;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Warning: Failed to copy file from " << src << " to " << dst << ": " << e.what() << std::endl;
        }
    }
}


void CamService::createListeningPipe() {
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        std::string errstring = strerror(errno);
        if (errstring.find("File exists") != std::string::npos) {
            std::filesystem::remove(PIPE_NAME);
            createListeningPipe();
        } else {
            std::cerr << "Error creating named pipe: " << strerror(errno) << std::endl;
            exit(1);
        }
    }
    std::cout << "Named pipe created at " << PIPE_NAME << std::endl;
}

void CamService::removeListeningPipe() {
    if (unlink(PIPE_NAME) == -1) {
        std::cerr << "Error removing named pipe: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::cout << "Named pipe removed" << std::endl;
}

void CamService::listenOnPipe() {
    debugPrint("Starting listenOnPipe()");
    std::string receivedMessage;
    std::regex saveRegex("save:(\\d+)");

    while(running) {
        std::ifstream pipe(PIPE_NAME);
        if (!pipe) {
            std::cerr << "Error opening named pipe: " << strerror(errno) << std::endl;
            continue;
        }
        std::getline(pipe, receivedMessage); // blocking read on pipe
        std::cout << "Received message on pipe: " << receivedMessage << std::endl;
        std::smatch match;
        if (receivedMessage == "kill") {
            killMainLoop();
            break;
        }
        else if (std::regex_match(receivedMessage, match, saveRegex)) { // save:<seconds to save>
            int value = std::stoi(match[1].str());
            saveRecordings(value);
        }
        pipe.close(); // close the pipe after reading
    }
    debugPrint("Closing listenOnPipe()");
}

void CamService::prepDirForService() {
    makeDir(this->recordingDir.c_str());
    makeDir(this->recordingSaveDir.c_str());

    // delete any segment*.ts or livestream.m3u8
    static const std::regex segmentRegex(R"(segment\d*\.ts)");

    for (const auto& entry : std::filesystem::directory_iterator(this->recordingDir)) {
        std::string filename = entry.path().filename().string();
        if(entry.is_regular_file() && (   filename.find("livestream.m3u8") != std::string::npos
                                    ||  std::regex_search(filename, segmentRegex)  )) {
                                        std::filesystem::remove(entry.path());
                                    }
    }
}
