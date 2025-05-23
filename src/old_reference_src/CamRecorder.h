#pragma once

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

class CamRecorder {
    public:
        CamRecorder();  // Constructor
        ~CamRecorder(); // Destructor

        void recordingLoop();
        void startRecording();
        void stopRecording();
        void saveRecordings(int seconds_back_to_save);
    private:
        bool isRecording;
        std::string recordingDir, recordingSaveDir;
        cv::VideoCapture cap;
        cv::VideoWriter writer;
        double height, width;
        std::thread recordingThread;
        std::thread cleanupThread;
        std::string currentlyRecordingVideoName;
        std::mutex writeMutex;


        void makeRecordingDirs();
        void createListeningPipe();
        void removeListeningPipe();
        void listenOnPipe();
        void cleanupThreadLoop();
        void deleteOlderFiles(std::time_t threshold_time);
        std::vector<std::filesystem::directory_entry> getRecordingDirContents();
        cv::VideoWriter setupNewVideoWriter();
        // Add private member variables or helper functions here
        
};