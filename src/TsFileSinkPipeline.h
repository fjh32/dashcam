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

class TsFileSinkPipeline : virtual public RecordingPipeline {
    protected:
        /// @brief Uses this->gstData->tee and adds a queue that creates 
        /// .ts video files of this->video_duration seconds with or without matching .m3u8 playlists
        /// @param make_playlist_file whether to make .m3u8 playlist alongside every .ts file
        void setupFileSinkElements(bool);
};