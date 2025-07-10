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

#include "RecordingPipeline.h"

class V4l2PipelineSource : public PipelineSource {
    
    public:
        void wait_for_video_device();
        void setupSource(GstElement* pipeline) override;
        GstPad* getSourcePad() override;
        GstElement* getTee() override;

    private:
        GstElement *source = nullptr;
        GstElement *queue = nullptr;
        GstElement *capsfilter = nullptr;
        GstElement *nv12_capsfilter = nullptr;
        GstElement *videoconvert = nullptr;
        GstElement *encoder = nullptr;
        GstElement *parser = nullptr;
        GstElement *tee = nullptr;

};