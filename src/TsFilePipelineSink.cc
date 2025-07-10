#include "TsFilePipelineSink.h"

static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    TsFilePipelineSink* instance = static_cast<TsFilePipelineSink*>(user_data);

    int index = instance->segmentIndex;
    instance->segmentIndex++;
    if (instance->segmentIndex >= instance->maxSegments) {
        instance->segmentIndex = 0;
    }
    instance->saveSegmentIndexToDisk();

    std::string ts_filename = "output_" + std::to_string(index) + ".ts";
    std::string ts_filepath = instance->context->recordingDir + "/" + ts_filename;
    instance->context->currentlyRecordingVideoName = ts_filepath;

    std::cout << "Recording new video: " << ts_filepath << std::endl;
    return g_strdup(ts_filepath.c_str());
}

// callback function to create new filename for splitmuxsink
static gchar* make_new_filename_with_playlist_file(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    TsFilePipelineSink* instance = static_cast<TsFilePipelineSink*>(user_data);

    gchar * gfilename = make_new_filename(splitmux, fragment_id, user_data);

    // Create matching .m3u8 playlist file
    string ts_filepath(gfilename);
    string ts_filename = filesystem::path(ts_filepath).filename().string();

    string m3u8_filename = ts_filename.substr(0, ts_filename.find_last_of(".")) + ".m3u8";
    string m3u8_filepath = instance->context->recordingDir + "/" + m3u8_filename;

    ofstream playlist(m3u8_filepath);
    if (playlist.is_open()) {
        playlist << "#EXTM3U\n";
        playlist << "#EXT-X-VERSION:3\n";
        playlist << "#EXT-X-TARGETDURATION:" << instance->context->video_duration << "\n";
        playlist << "#EXT-X-MEDIA-SEQUENCE:0\n";
        playlist << "#EXTINF:" << instance->context->video_duration << ".0,\n";
        playlist << "/recordings/" << ts_filename << "\n";
        playlist << "#EXT-X-ENDLIST\n";
        playlist.close();
        cout << "Generated HLS playlist: " << m3u8_filepath << endl;
    } else {
        cerr << "Error creating HLS playlist file: " << m3u8_filepath << endl;
    }

    return gfilename; // GStreamer will free this string
}

int get_next_segment_index(const std::string& dir, int max_segments) {
    int highest = -1;
    static const std::regex pattern(R"(output_(\d+)\.ts)");

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const std::string filename = entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, pattern)) {
            try {
                int n = std::stoi(match[1]);
                if (n > highest && n < max_segments) {
                    highest = n;
                }
            } catch (...) {
                continue;
            }
        }
    }

    int next = highest + 1;
    return (next >= max_segments) ? 0 : next;
}

////////////////////////////////////////
TsFilePipelineSink::TsFilePipelineSink(RecordingPipeline* context, bool makePlaylist)
    : context(context), makePlaylist(makePlaylist), maxSegments(MAX_SEGMENTS){
        this->loadCurrentSegmentIndexFromDisk();
    }

TsFilePipelineSink::TsFilePipelineSink(RecordingPipeline* context, bool makePlaylist, int max_video_segments)
    : context(context), makePlaylist(makePlaylist), maxSegments(max_video_segments) {
        this->loadCurrentSegmentIndexFromDisk();
    }

GstPad* TsFilePipelineSink::getSinkPad() {
    return gst_element_get_static_pad(queue, "sink");
}

GstElement* TsFilePipelineSink::getSinkElement() {
    return sink;  // this->sink is the splitmuxsink
}

void TsFilePipelineSink::setupSink(GstElement* pipeline) {
    queue = gst_element_factory_make("queue", "file_sink_queue");
    muxer = gst_element_factory_make("mpegtsmux", "muxer");
    sink = gst_element_factory_make("splitmuxsink", "sink");
    if (!queue || !muxer || !sink) {
        std::cerr << "Failed to create one or more sink elements.\n";
        exit(1);
    }

    g_object_set(sink, "muxer", muxer, NULL);
    g_object_set(sink, "max-size-time", (guint64)context->video_duration * GST_SECOND, NULL);

    if(this->makePlaylist) {
        g_signal_connect(sink, "format-location", G_CALLBACK(make_new_filename_with_playlist_file), this);
    }
    else {
        g_signal_connect(sink, "format-location", G_CALLBACK(make_new_filename), this);
    }

    gst_bin_add_many(GST_BIN(pipeline), queue, sink, nullptr);
    if (!gst_element_link(queue, sink)) {
        std::cerr << "Failed to link file_sink_queue -> splitmuxsink\n";
        exit(1);
    }

    GstPad *tee_video_pad = gst_element_request_pad_simple(context->getSourceTee(), "src_%u");
    GstPad *queue_video_pad = gst_element_get_static_pad (queue, "sink");
    if (gst_pad_can_link(tee_video_pad, queue_video_pad)) {
        if (gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
            std::cout << "Error: Could not link tee and queue pads." << std::endl;
            exit(1);
        }
    } else {
        std::cout << "Error: Tee and queue pads are not compatible." << std::endl;
        exit(1);
    }

    // gst_element_release_request_pad (gstData->tee, tee_video_pad);
    gst_object_unref(tee_video_pad);
    gst_object_unref(queue_video_pad);
    std::cout << "File sink elements setup successfully.\n";
}

void TsFilePipelineSink::saveSegmentIndexToDisk() {
    std::ofstream f(context->recordingDir + SEGMENT_INDEX_FILE, std::ios::trunc);
    if (f.is_open()) {
        f << segmentIndex;
        f.close();
    }
}

void TsFilePipelineSink::loadCurrentSegmentIndexFromDisk() {
    segmentIndex = 0;  // Default value if file is missing or invalid

    std::ifstream f(context->recordingDir + SEGMENT_INDEX_FILE);
    if (f.is_open()) {
        int value;
        if (f >> value && value >= 0 && value < maxSegments) {
            segmentIndex = value;
        } else {
            std::cerr << "Warning: Invalid segment index file contents. Defaulting to 0.\n";
        }
        f.close();
    } else {
        std::cout << "Segment index file not found. Starting from 0.\n";
    }
}
