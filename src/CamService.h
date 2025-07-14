#pragma once

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

#include "RecordingPipeline.h"
#include "V4l2PipelineSource.h"
#include "LibcameraPipelineSource.h"
#include "TsFilePipelineSink.h"
#include "HlsPipelineSink.h"

using namespace std;

#define PIPE_NAME "/tmp/camrecorder.pipe"

#ifdef DEBUG
#define VIDEO_DURATION 2
#define RECORDING_DIR "./recordings/"
#define RECORDING_SAVE_DIR "./recordings/save/"
#define DELETE_OLDER_THAN 60 * 60 * 24 * 1 // 1 days
#define SEGMENTS_TO_KEEP 86400 / VIDEO_DURATION * 2 // 2 days
#else
#define VIDEO_DURATION  2 
#define RECORDING_DIR "/var/lib/dashcam/recordings/"
#define RECORDING_SAVE_DIR "/var/lib/dashcam/recordings/save/"
#define DELETE_OLDER_THAN 60 * 60 * 24 * 1 // 1 days
#define SEGMENTS_TO_KEEP 86400 / VIDEO_DURATION * 2 // 2 days
#endif

class CamService {
    public:
        CamService(int* argc, char** argv[]);
        ~CamService();

        void mainLoop();
        void killMainLoop();
    private:
        bool running;
        std::unique_ptr<RecordingPipeline> recordingPipeline;
        std::mutex recordingSwapMutex;
        std::chrono::_V2::steady_clock::time_point currentVideoStartTime;
        string recordingSaveDir, recordingDir;

        void recordingLoop();
        void createListeningPipe();
        void listenOnPipe();
        void removeListeningPipe();
        void makeRecordingDirs();
        void prepDirForService();

        bool isRecording();
        void stopRecording();
        void startRecording();
        void saveRecordings(int seconds_back_to_save);
};