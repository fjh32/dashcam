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
    // gstData->source = gst_element_factory_make("v4l2src", "source");
    gstData->encoder = gst_element_factory_make("v4l2h264enc", "encoder");
    #else
    debugPrint("Creating v4l2src source");
    gstData->source = gst_element_factory_make("v4l2src", "source");
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    g_object_set(gstData->encoder, "key-int-max", FRAME_RATE, NULL);
    #endif
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    gstData->muxer = gst_element_factory_make("matroskamux", "muxer");
    gstData->sink = gst_element_factory_make("splitmuxsink", "sink");

    if(!gstData->pipeline || !gstData->source || !gstData->queue || !gstData->capsfilter || !gstData->videoconvert || !gstData->encoder || !gstData->muxer || !gstData->sink) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }
    
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        #ifdef RPI_MODE
        "format", G_TYPE_STRING, "NV12",
        #else
        "format", G_TYPE_STRING, "YUY2",
        #endif
        "width", G_TYPE_INT, VIDEO_WIDTH,
        "height", G_TYPE_INT, VIDEO_HEIGHT,
        "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
        nullptr);
    g_object_set(gstData->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    g_object_set(gstData->sink, "muxer", gstData->muxer, NULL);
    g_object_set(gstData->sink, "max-size-time", (guint64)video_duration * GST_SECOND, NULL); // 30 minutes
    g_signal_connect(gstData->sink, "format-location", G_CALLBACK(make_new_filename), this);

    // muxer already added to splitmuxsink, so don't add it to bin or link it
    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder,  gstData->sink, NULL);
    if(!gst_element_link_many(gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder,  gstData->sink, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
    cout << "Gstreamer elements setup successfully." << endl;
}