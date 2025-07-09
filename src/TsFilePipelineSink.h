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
static gchar* make_new_filename_with_playlist_file(GstElement *splitmux, guint fragment_id, gpointer user_data);

class TsFilePipelineSink : public PipelineSink {
    public:
        TsFilePipelineSink(RecordingPipeline* context, bool makePlaylist);
        
        GstPad* getSinkPad() override;
        GstElement* getSinkElement() override;
        void setupSink(GstElement* pipeline) override;
        
    private:
        RecordingPipeline* context;
        bool makePlaylist;
        GstElement* queue = nullptr;
        GstElement* muxer = nullptr;
        GstElement* sink = nullptr;
        GstPad* tee_pad = nullptr;
};