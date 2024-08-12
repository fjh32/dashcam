#pragma once

#include <gst/gst.h>
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

#define FRAME_RATE 10
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

class GstData {
    public:
        GstData();
        ~GstData();

        GstElement *pipeline, *source, *queue, *capsfilter, *videoconvert, *encoder, *muxer, *sink;
        GstBus *bus;
};

class GstRecordingPipeline {
    public:
    // in the future, provide an optional gstreamer source to this pipeline
        GstRecordingPipeline(const char dir[], int* argc, char** argv[]);  // Constructor
        ~GstRecordingPipeline(); // Destructor

        bool pipelineRunning;
        bool pipelineKilled;

        void startPipeline();
        void stopPipeline();

    private:
        unique_ptr<GstData> gstData;
        string recordingDir;
        string currentlyRecordingVideoName;
        std::thread pipelineThread;
        void pipelineRunner();
        bool handleBusMessage(GstBus *bus);
        void setupNewVideoSink();
        void setupGstElements();
        string make_new_video_name();
};