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

static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data);

class TsFileSinkPipeline : virtual public RecordingPipeline {
    protected:
        // TsFileSinkPipeline(const char* dir, int vid_duration, int* argc, char*** argv)
        //     : RecordingPipeline(dir, vid_duration, argc, argv) {}
        void setupFileSinkElements();
};