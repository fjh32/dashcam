#pragma once

#include <gst/gst.h>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <sys/stat.h>
#include <thread>
#include <fstream>
#include <filesystem>
#include <regex>
#include <mutex>
#include "utilities.h"
#include <algorithm>


#define FRAME_RATE 10

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

class GstData {
    public:
        GstData();
        ~GstData();

        GstElement *pipeline, *source, *queue, *capsfilter, *videoconvert, *encoder, *muxer, *sink;
        GstBus *bus;
};


class CameraRecorder {
    public:
        CameraRecorder(int argc, char* argv[]);  // Constructor
        ~CameraRecorder(); // Destructor

        void startRecording();
        void startPipeline();
        void stopRecording();
        void stopPipeline();
        void recordingLoop();
        void kill();
        void mainLoop();

    private:
        bool isRecording, killed, pipelineRunning;
        std::unique_ptr<GstData> gstData;
        std::string currentlyRecordingVideoName;
        std::string recordingDir, recordingSaveDir;

        std::mutex recordingSwapMutex;
        std::chrono::_V2::steady_clock::time_point currentVideoStartTime;
        std::thread pipelineThread, recordingThread, cleanupThread;
        
        void setupGstElements(int argc, char* argv[]);
        void saveRecordings(int seconds_back_to_save);
        std::string makeNewFilename();
        void createListeningPipe();
        void listenOnPipe();
        void removeListeningPipe();
        bool handleBusMessage(GstMessage *msg);
        void makeRecordingDirs();
        void cleanupThreadLoop();
        void deleteOlderFiles(std::time_t threshold_time);
        std::vector<std::filesystem::directory_entry> getRecordingDirContents();
};