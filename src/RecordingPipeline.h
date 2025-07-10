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

class PipelineSource {
    public:
        virtual GstPad* getSourcePad() = 0;
        virtual GstElement* getTee() = 0;
        virtual void setupSource(GstElement* pipeline) = 0;
        virtual ~PipelineSource() = default;
};

class PipelineSink {
    public:
        virtual GstPad* getSinkPad() = 0;
        virtual GstElement* getSinkElement() = 0;
        virtual void setupSink(GstElement* pipeline) = 0;
        virtual ~PipelineSink() = default;
};

class RecordingPipeline {
    public:
        RecordingPipeline(const char dir[], int vid_duration, int* argc, char*** argv);
        ~RecordingPipeline();

        void setSource(std::unique_ptr<PipelineSource> source);
        void addSink(std::unique_ptr<PipelineSink> sink);

        void startPipeline();
        void stopPipeline();
        void createNewVideo();

        GstElement* getSourceTee(); // expose for PipelineSinks

        bool pipelineRunning;
        bool pipelineKilled;
        std::string recordingDir;
        int video_duration;
        std::string currentlyRecordingVideoName;
    private:
        GstElement* pipeline = nullptr;
        GstBus* bus = nullptr;

        std::unique_ptr<PipelineSource> source;
        std::vector<std::unique_ptr<PipelineSink>> sinks;

        std::thread pipelineThread;

        void buildPipeline();
        void pipelineRunner();
        bool handleBusMessage(GstBus *bus);
};