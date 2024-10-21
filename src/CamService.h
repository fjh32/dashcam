#pragma once

#include "RecordingPipeline.h"
#include "HttpServer.h"

#include <string>
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <regex>
#include <mutex>
#include <algorithm>
#include <signal.h>
#include "utilities.h"

using namespace std;

#define PIPE_NAME "/tmp/camrecorder.pipe"

#ifdef DEBUG
#define VIDEO_DURATION 300 
#define WEB_DIR "./dashcam_web_static/"
#define WEB_PORT 8888
#define WEBSITE_ROOT "192.168.1.70"
#define RECORDING_DIR "./recordings/"
#define RECORDING_SAVE_DIR "./recordings/save/"
#define DELETE_OLDER_THAN 600 
#else
#define VIDEO_DURATION  1800 
#define WEB_DIR "./dashcam_web_static/"
#define WEB_PORT 80
#define WEBSITE_ROOT "https://ripplein.space/"
#define RECORDING_DIR "/home/recordings/"
#define RECORDING_SAVE_DIR "/home/recordings/save/"
#define DELETE_OLDER_THAN 86400
#endif

class CamService {
    public:
        CamService(int* argc, char** argv[]);  // Constructor
        ~CamService(); // Destructor

        void mainLoop();
        void killMainLoop();
    private:
        bool running;
        std::unique_ptr<RecordingPipeline> recordingPipeline;
        std::mutex recordingSwapMutex;
        std::chrono::_V2::steady_clock::time_point currentVideoStartTime;
        std::thread cleanupThread;
        string recordingSaveDir, recordingDir;

        unique_ptr<HttpServer> httpServer;

        void recordingLoop();
        void createListeningPipe();
        void listenOnPipe();
        void removeListeningPipe();
        void makeRecordingDirs();
        void cleanupThreadLoop();
        void deleteOlderFiles(std::time_t threshold_time);

        bool isRecording();
        void stopRecording();
        void startRecording();
        void saveRecordings(int seconds_back_to_save);
};