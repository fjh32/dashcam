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
    recordingPipeline->addSink(std::make_unique<TsFilePipelineSink>(recordingPipeline.get(), false,));
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
    if(seconds_back_to_save <= VIDEO_DURATION) {
        std::string filename = recordingPipeline->currentlyRecordingVideoName;
        std::string baseFilename = filename.substr(filename.find_last_of("/\\") + 1);
        std::string savePath = recordingSaveDir + baseFilename;
        recordingPipeline->createNewVideo();
        std::filesystem::copy_file(filename, savePath, std::filesystem::copy_options::overwrite_existing);
        std::cout << "Saved file: " << savePath << std::endl;

    } else {
        auto dir_contents = getDirContents(this->recordingDir);

        // dir_contents.erase(std::remove_if(dir_contents.begin(), dir_contents.end(), [](const std::filesystem::directory_entry &entry) {
        //     const std::string filename = entry.path().filename().string();
        //     return filename.find(".mp4") == std::string::npos ;
        // }), dir_contents.end());

        for(const auto& entry :dir_contents) {
            std::time_t file_timestamp = time_t_from_direntry(entry);
            if (file_timestamp > threshold_time) {
                    std::string filename = entry.path().filename().string();

                    if (recordingPipeline->currentlyRecordingVideoName.find(filename) != std::string::npos) {
                        recordingPipeline->createNewVideo();
                    }

                    std::string savePath = recordingSaveDir + filename;
                    std::filesystem::copy_file(entry.path(), savePath, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Saved file: " << filename << std::endl;
            }
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
