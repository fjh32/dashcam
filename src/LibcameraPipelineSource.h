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

class LibcameraPipelineSource : public PipelineSource {
    
    public:
        void setupSource(GstElement* pipeline) override;
        GstPad* getSourcePad() override;
        GstElement* getTee() override;

    private:
        GstElement *source = nullptr;
        GstElement *encoder = nullptr;
        GstElement *queue = nullptr;
        GstElement *capsfilter = nullptr;
        GstElement *videoconvert = nullptr;
        GstElement *videoflip = nullptr;
        GstElement *parser = nullptr;
        GstElement *tee = nullptr;

};