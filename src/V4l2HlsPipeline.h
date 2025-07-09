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

#include "HlsPipeline.h"
#include "TsFileSinkPipeline.h"

class V4l2HlsPipeline : public HlsPipeline, public TsFileSinkPipeline {
    public:
        V4l2HlsPipeline(const char dir[], int vid_duration, int* argc, char*** argv);
    
    protected:
        void setupRecordingPipeline() override;

    private:
        void setupV4l2Recording();
        void wait_for_video_device();
};