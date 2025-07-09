#pragma once

#include <gst/gst.h>
#include <gst/video/video-format.h>

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

#ifdef DEBUG
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define FRAME_RATE 10
#else
#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define FRAME_RATE 15
#endif

#define HLS_FILE_ROOT "/home/frank/livestream/"

static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data);

class GstData {
    public:
        GstData();
        ~GstData();
        GstElement *pipeline; 
        GstElement *extra_element;
        GstElement *source, *queue, *capsfilter, *videoconvert, *encoder, *tee; 
        GstElement *post_encode_caps, *h264parser;
        GstElement *muxer, *file_sink_queue, *sink;
        GstElement  *hls_queue, *h264parse, *hlsmux, *hlssink;
        GstElement *videoflip;
        GstBus *bus;
};

class RecordingPipeline {
    public:
        // in the future, provide an optional gstreamer source to this pipeline
        RecordingPipeline(const char dir[], int vid_duration, int* argc, char** argv[]); 
        ~RecordingPipeline();

        bool pipelineRunning;
        bool pipelineKilled;
        string recordingDir;
        int video_duration;
        string currentlyRecordingVideoName;

        void startPipeline();
        void stopPipeline();
        void createNewVideo();

    private:
        unique_ptr<GstData> gstData;
        std::thread pipelineThread;
        std::string webroot;
        void pipelineRunner();
        bool handleBusMessage(GstBus *bus);
        void setupGstElements();
        void setupHlsElements();
        void setupFileSinkElements();
        void setupSoftwareEncodingRecorder();
        void setupHardwareEncodingRecorder();
        void setupV4l2Recording();
        void setupV4l2RecordingMJPG();
        
};