#pragma once

#include "RecordingPipeline.h"

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
#define VIDEO_DURATION 300 // 10 mins in seconds
#define RECORDING_DIR "./recordings/"
#define RECORDING_SAVE_DIR "./recordings/save/"
#define DELETE_OLDER_THAN 600 // 1 hour in seconds
#else
#define VIDEO_DURATION  1800 // in seconds
#define RECORDING_DIR "/home/recordings/"
#define RECORDING_SAVE_DIR "/home/recordings/save/"
#define DELETE_OLDER_THAN 86400 // 24 hours in seconds
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
        std::vector<std::filesystem::directory_entry> getRecordingDirContents();

        
        
};