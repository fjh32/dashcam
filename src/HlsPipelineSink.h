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


class HlsPipelineSink : public PipelineSink {
    public:
        HlsPipelineSink(RecordingPipeline* context);
        
        GstPad* getSinkPad() override;
        GstElement* getSinkElement() override;
        void setupSink(GstElement* pipeline) override;
        
    private:
        RecordingPipeline* context;
        GstElement* queue = nullptr;
        GstElement* parser = nullptr;
        GstElement* mux = nullptr;
        GstElement* sink = nullptr;
        GstPad* tee_pad = nullptr;

        std::string webroot;
};