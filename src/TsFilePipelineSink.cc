#include "TsFilePipelineSink.h"

static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    RecordingPipeline* instance = static_cast<RecordingPipeline*>(user_data);

    // Create filename for .ts
    string ts_filename = "output_" + formatted_time() + ".ts";
    string ts_filepath = instance->recordingDir + "/" + ts_filename;
    instance->currentlyRecordingVideoName = ts_filepath;

    cout << "Recording new video: " << ts_filepath << endl;
    return g_strdup(ts_filepath.c_str());
}

// callback function to create new filename for splitmuxsink
static gchar* make_new_filename_with_playlist_file(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    RecordingPipeline* instance = static_cast<RecordingPipeline*>(user_data);

    gchar * gfilename = make_new_filename(splitmux, fragment_id, user_data);

    // Create matching .m3u8 playlist file
    string ts_filepath(gfilename);
    string ts_filename = filesystem::path(ts_filepath).filename().string();

    string m3u8_filename = ts_filename.substr(0, ts_filename.find_last_of(".")) + ".m3u8";
    string m3u8_filepath = instance->recordingDir + "/" + m3u8_filename;

    ofstream playlist(m3u8_filepath);
    if (playlist.is_open()) {
        playlist << "#EXTM3U\n";
        playlist << "#EXT-X-VERSION:3\n";
        playlist << "#EXT-X-TARGETDURATION:" << instance->video_duration << "\n";
        playlist << "#EXT-X-MEDIA-SEQUENCE:0\n";
        playlist << "#EXTINF:" << instance->video_duration << ".0,\n";
        playlist << "/recordings/" << ts_filename << "\n";
        playlist << "#EXT-X-ENDLIST\n";
        playlist.close();
        cout << "Generated HLS playlist: " << m3u8_filepath << endl;
    } else {
        cerr << "Error creating HLS playlist file: " << m3u8_filepath << endl;
    }

    return gfilename; // GStreamer will free this string
}

////////////////////////////////////////
TsFilePipelineSink::TsFilePipelineSink(RecordingPipeline* context, bool makePlaylist)
    : context(context), makePlaylist(makePlaylist) {}

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
        g_signal_connect(sink, "format-location", G_CALLBACK(make_new_filename_with_playlist_file), this->context);
    }
    else {
        g_signal_connect(sink, "format-location", G_CALLBACK(make_new_filename), this->context);
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
////////////////////////////////////////

// void TsFileSinkPipeline::setupFileSinkElements(bool make_playlist_file) {
//     gstData->file_sink_queue = gst_element_factory_make("queue", "file_sink_queue");
//     if (!gstData->file_sink_queue) {
//         std::cerr << "Error: Failed to queue in setupFileSinkElements()." << std::endl;
//         exit(1);
//     }
//     // gstData->muxer = gst_element_factory_make("mp4mux", "muxer");
//     gstData->muxer = gst_element_factory_make("mpegtsmux", "muxer");  // MPEG-TS muxer
//     if (!gstData->muxer) {
//         std::cerr << "Error: Could not create 'mpegtsmux'. Plugin missing or GStreamer misconfigured?" << std::endl;
//         exit(1);
//     }

//     gstData->sink = gst_element_factory_make("splitmuxsink", "sink");
//     if ( !gstData->sink ) {
//         std::cerr << "Error: Failed to create splitmuxsink." << std::endl;
//         exit(1);
//     }
//     g_object_set(gstData->sink, "muxer", gstData->muxer, NULL);
//     g_object_set(gstData->sink, "max-size-time", (guint64)this->video_duration * GST_SECOND, NULL); // 30 minutes

//     if(make_playlist_file) {
//         g_signal_connect(gstData->sink, "format-location", G_CALLBACK(make_new_filename_with_playlist_file), this);
//     }
//     else {
//         g_signal_connect(gstData->sink, "format-location", G_CALLBACK(make_new_filename), this);
//     }

//     gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->file_sink_queue, gstData->sink, NULL);
//     if(!gst_element_link_many(gstData->file_sink_queue, gstData->sink, NULL)) {
//         std::cout << "Error: Could not link gstreamer filesink elements." << std::endl;
//         exit(1);
//     }
//     // link tee to file_sink_queue
//     GstPad *tee_video_pad = gst_element_request_pad_simple (gstData->tee, "src_%u");
//     GstPad *queue_video_pad = gst_element_get_static_pad (gstData->file_sink_queue, "sink");
//     if (gst_pad_can_link(tee_video_pad, queue_video_pad)) {
//         if (gst_pad_link(tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK) {
//             std::cout << "Error: Could not link tee and queue pads." << std::endl;
//             exit(1);
//         }
//     } else {
//         std::cout << "Error: Tee and queue pads are not compatible." << std::endl;
//         exit(1);
//     }

//     // gst_element_release_request_pad (gstData->tee, tee_video_pad);
//     gst_object_unref(tee_video_pad);
//     gst_object_unref(queue_video_pad);
//     std::cout << "File sink elements setup successfully.\n";
// }