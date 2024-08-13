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
#define VIDEO_DURATION 60
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480

static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data);

class GstData {
    public:
        GstData();
        ~GstData();

        GstElement *pipeline, *source, *queue, *capsfilter, *videoconvert, *encoder, *muxer, *sink;
        GstBus *bus;
};

class RecordingPipeline {
    public:
    // in the future, provide an optional gstreamer source to this pipeline
        RecordingPipeline(const char dir[], int vid_duration, int* argc, char** argv[]);  // Constructor
        ~RecordingPipeline(); // Destructor

        bool pipelineRunning;
        bool pipelineKilled;
        string recordingDir;
        string currentlyRecordingVideoName;

        void startPipeline();
        void stopPipeline();
        void createNewVideo();

    private:
        unique_ptr<GstData> gstData;
        
        int video_duration;
        std::thread pipelineThread;
        void pipelineRunner();
        bool handleBusMessage(GstBus *bus);
        void setupNewVideoSink();
        void setupGstElements();
        string make_new_video_name();
        gchar* gst_create_new_video_name();
};