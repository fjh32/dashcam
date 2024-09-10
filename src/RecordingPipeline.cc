#include "RecordingPipeline.h"


using namespace std;

// callback function to create new filename for splitmuxsink
static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    RecordingPipeline* instance = static_cast<RecordingPipeline*>(user_data);
    string filepath = instance->recordingDir + "/output_" + formatted_time() + ".mkv";
    instance->currentlyRecordingVideoName = filepath;
    cout << "Recording new video: " << filepath << endl;
    return g_strdup(filepath.c_str()); // Return the new filename (must be dynamically allocated since GStreamer will free it)
}

/// GstData class
GstData::GstData() {}
GstData::~GstData() {}

/// RecordingPipeline class
RecordingPipeline::RecordingPipeline( const char dir[], int vid_duration, int* argc, char** argv[]) 
                            : recordingDir(dir), video_duration(vid_duration), pipelineRunning(false), currentlyRecordingVideoName("None") {
    debugPrint("Creating RecordingPipeline object");
    gst_init(argc, argv);

    makeDir(recordingDir.c_str());

    gstData = make_unique<GstData>();
    setupGstElements();
}

RecordingPipeline::~RecordingPipeline() {
    debugPrint("Destroying RecordingPipeline object");
    if(pipelineRunning) {
        // Gstreamer needs explicit cleanup
        // need to explicitly call stopPipeline before the destructor is called
        #ifdef RPI_MODE
        cout << "******* WARNING *******" << endl;
        cout << "Failed to call stopPipeline() before Destructor was called" << endl;
        cout << "This appears to be an issue setting pipeline state to NULL in a destructor called during program exit with libcamera source on the Raspberry Pi" << endl;
        cout << "About to segfault..." << endl;
        cout << "***** END WARNING *****" << endl;
        #endif
        stopPipeline(); // this seems to segfault when using libcamera source on rpi
    }
    debugPrint("Unref'ing pipeline");
    gst_object_unref(gstData->pipeline);
}

void RecordingPipeline::pipelineRunner() {
    debugPrint("startPipeline() at " + formatted_time());

    if(!pipelineRunning) {
        debugPrint("Starting pipeline");

        gst_element_set_state(gstData->pipeline, GST_STATE_PLAYING);
        pipelineRunning = true;
        

        gstData->bus = gst_element_get_bus(gstData->pipeline);
        while(handleBusMessage(gstData->bus)) {}
        gst_object_unref(gstData->bus);

        pipelineRunning = false;
    }

    debugPrint("exiting startPipeline()");
}

void RecordingPipeline::startPipeline() {
    pipelineThread = std::thread(&RecordingPipeline::pipelineRunner, this);
}

void RecordingPipeline::createNewVideo() {
    debugPrint("Received createNewVideo() signal.");
    g_signal_emit_by_name (gstData->sink, "split-now");
}

void RecordingPipeline::stopPipeline() {
    debugPrint("stopPipeline()");

    if(pipelineRunning) {
        debugPrint("Sending EOS event");
        gst_element_send_event(gstData->pipeline, gst_event_new_eos());
        pipelineThread.join();
        debugPrint("Setting pipeline state to NULL");
        gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
    }

    debugPrint("exiting stopPipeline()");
}

bool RecordingPipeline::handleBusMessage(GstBus *bus) {
    bool keepRunning = true;
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 500*GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if(msg) {
        GError *err;
        gchar *debug_info;
        switch(GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                cout << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << endl;
                cout << "Debugging information: " << (debug_info ? debug_info : "none") << endl;
                g_clear_error(&err);
                g_free(debug_info);
                keepRunning = false;
                break;
            case GST_MESSAGE_EOS:
                cout << "End-Of-Stream reached." << endl;
                keepRunning = false;
                break;
            default:
                cout << "Unexpected message received." << endl;
                break;
        }
        gst_message_unref(msg);
    }
    return keepRunning;
}

void RecordingPipeline::setupGstElements() {
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    #ifdef RPI_MODE
    debugPrint("Creating libcamerasrc source");
    gstData->source = gst_element_factory_make("libcamerasrc", "source");
    // gstData->encoder = gst_element_factory_make("v4l2h264enc", "encoder");
    #else
    debugPrint("Creating v4l2src source");
    gstData->source = gst_element_factory_make("v4l2src", "source");
    #endif
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");

    gstData->tee = gst_element_factory_make ("tee", "tee");



    if(!gstData->pipeline || !gstData->source || !gstData->queue || !gstData->capsfilter || !gstData->videoconvert || !gstData->encoder || !gstData->muxer || !gstData->tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }
    
    g_object_set(gstData->encoder, "key-int-max", FRAME_RATE, NULL); // set keyframe interval to framerate

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        // #ifdef RPI_MODE
        // "format", G_TYPE_STRING, "NV12",
        // #else
        "format", G_TYPE_STRING, "YUY2",
        // #endif
        "width", G_TYPE_INT, VIDEO_WIDTH,
        "height", G_TYPE_INT, VIDEO_HEIGHT,
        "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
        nullptr);
    g_object_set(gstData->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // muxer already added to splitmuxsink, so don't add it to bin or link it
    gst_bin_add_many(GST_BIN(gstData->pipeline), 
                            gstData->source, 
                            gstData->queue, 
                            gstData->capsfilter, 
                            gstData->videoconvert, 
                            gstData->encoder,  
                            // gstData->sink, 
                            gstData->tee, 
                            NULL);
    if(!gst_element_link_many(gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder,  gstData->tee, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }

    setupFileSinkElements();
    setupHlsElements();

    cout << "Gstreamer elements setup successfully." << endl;
}

void RecordingPipeline::setupFileSinkElements() {
    gstData->file_sink_queue = gst_element_factory_make("queue", "file_sink_queue");
    gstData->muxer = gst_element_factory_make("matroskamux", "muxer");
    gstData->sink = gst_element_factory_make("splitmuxsink", "sink");
    
    g_object_set(gstData->sink, "muxer", gstData->muxer, NULL);
    g_object_set(gstData->sink, "max-size-time", (guint64)video_duration * GST_SECOND, NULL); // 30 minutes
    g_signal_connect(gstData->sink, "format-location", G_CALLBACK(make_new_filename), this);

    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->file_sink_queue, gstData->sink, NULL);
    if(!gst_element_link_many(gstData->file_sink_queue, gstData->sink, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }

    // link tee to file_sink_queue
    GstPad *tee_video_pad = gst_element_request_pad_simple (gstData->tee, "src_%u");
    GstPad *queue_video_pad = gst_element_get_static_pad (gstData->file_sink_queue, "sink");
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
    debugPrint("File sink elements setup successfully.");
}
void RecordingPipeline::setupHlsElements() {
    gstData->hls_queue = gst_element_factory_make("queue", "hls_queue");
    gstData->h264parse = gst_element_factory_make("h264parse", "h264parse");
    gstData->hlssink = gst_element_factory_make("hlssink2", "hlssink");

    if (!gstData->hls_queue || !gstData->h264parse || !gstData->hlssink) {
        std::cout << "Error: Failed to create HLS elements." << std::endl;
        exit(1);
    }

    // TODO configurable location
    // Configure hlssink2 properties
    if (!std::filesystem::exists(HLS_FILE_ROOT)) {
        std::filesystem::create_directory(HLS_FILE_ROOT);
    }
    std::string livestream_location = recordingDir + "/" + "livestream.m3u8";
    std::string segment_location = recordingDir + "/" + "segment%05d.ts";

    g_object_set(G_OBJECT(gstData->hlssink),
                "playlist-length", 5,              // Reduced from 5
                "target-duration", 5,              // Reduced from 10
                "max-files", 5,
                "playlist-location", livestream_location.c_str(),
                "location", segment_location.c_str(),
                 NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(gstData->pipeline),
                     gstData->hls_queue,
                     gstData->h264parse,
                     gstData->hlssink,
                     NULL);

    // Link the elements
    if (!gst_element_link_many(gstData->hls_queue, gstData->h264parse, gstData->hlssink, NULL)) {
        std::cout << "Error: Could not link HLS elements." << std::endl;
        exit(1);
    }

    // Link tee to hls_queue
    GstPad *tee_hls_pad = gst_element_request_pad_simple(gstData->tee, "src_%u");
    GstPad *queue_hls_pad = gst_element_get_static_pad(gstData->hls_queue, "sink");

    if (gst_pad_link(tee_hls_pad, queue_hls_pad) != GST_PAD_LINK_OK) {
        std::cout << "Error: Could not link tee and HLS queue pads." << std::endl;
        exit(1);
    }

    gst_object_unref(queue_hls_pad);
    gst_object_unref(tee_hls_pad);

    std::cout << "HLS elements setup successfully." << std::endl;
}