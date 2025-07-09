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

class HlsPipeline : virtual public RecordingPipeline {

    protected:
        // HlsPipeline(const char* dir, int vid_duration, int* argc, char*** argv)
        //     : RecordingPipeline(dir, vid_duration, argc, argv) {}
        void setupHlsElements();
};