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

class V4l2PipelineSrc : virtual public RecordingPipeline {
    protected:
        void setupV4l2SrcAndTee();
        void wait_for_video_device();
};