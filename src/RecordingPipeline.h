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

class PipelineSrc {
    public:
        virtual GstElement* getSrcPad() = 0;
        virtual void setupSrc(GstElement* pipeline) = 0;
        virtual void ~PipelineSrc() = default;
};

class PipelineSink {
    public:
        virtual GstElement* getSinkPad() = 0;
        virtual void setupSink(GstElement* pipeline) = 0;
        virtual void ~PipelineSink() = default;
};

class RecordingPipeline {
    public:
        // in the future, provide an optional gstreamer source to this pipeline
        RecordingPipeline(const char dir[], int vid_duration, int* argc, char*** argv);
        ~RecordingPipeline();

        bool pipelineRunning;
        bool pipelineKilled;
        string recordingDir;
        int video_duration;
        string currentlyRecordingVideoName;

        void startPipeline();
        void stopPipeline();
        void createNewVideo();

    protected:
        virtual void setupRecordingPipeline() = 0;

        unique_ptr<GstData> gstData;
        std::thread pipelineThread;
        std::string webroot;

        void pipelineRunner();
        bool handleBusMessage(GstBus *bus);
};