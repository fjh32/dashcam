#include <GstRecordingPipeline.h>

using namespace std;

/// GstData class

GstData::GstData() {
    debugPrint("Creating GstData object");
}
GstData::~GstData() {
    debugPrint("Destroying GstData object");
}

/// GstRecordingPipeline class
GstRecordingPipeline::GstRecordingPipeline( const char dir[], int* argc, char** argv[]) 
                            : recordingDir(dir), pipelineRunning(false), currentlyRecordingVideoName("None") {
    debugPrint("Creating GstRecordingPipeline object");
    gst_init(argc, argv);

    makeDir(recordingDir.c_str());

    gstData = make_unique<GstData>();
    setupGstElements();
}

GstRecordingPipeline::~GstRecordingPipeline() {
    debugPrint("Destroying GstRecordingPipeline object");
    if(pipelineRunning) {
        // stopPipeline(); // this seems to segfault when using libcamera source on rpi
        gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
    }
    debugPrint("Unref'ing pipeline");
    gst_object_unref(gstData->pipeline);
}

void GstRecordingPipeline::startPipeline() {
    debugPrint("startPipeline()");

    if(!pipelineRunning) {
        debugPrint("Starting pipeline");

        setupNewVideoSink();
        gst_element_set_state(gstData->pipeline, GST_STATE_PLAYING);
        pipelineRunning = true;

        gstData->bus = gst_element_get_bus(gstData->pipeline);
        while(handleBusMessage(gstData->bus)) {}
        gst_object_unref(gstData->bus);
    }

    debugPrint("exiting startPipeline()");
}

void GstRecordingPipeline::stopPipeline() {
    debugPrint("stopPipeline()");

    if(pipelineRunning) {
        debugPrint("Stopping pipeline");
        debugPrint("Sending EOS event");
        gst_element_send_event(gstData->pipeline, gst_event_new_eos());
        debugPrint("Setting pipeline state to NULL");

        // GstState state;
        // GstState pending;

        // // Get the pipeline state, blocking until the state change is complete
        // GstStateChangeReturn ret = gst_element_get_state(gstData->pipeline, &state, &pending, GST_CLOCK_TIME_NONE);

        // if (ret == GST_STATE_CHANGE_SUCCESS) {
        // std::cout << "Pipeline is now in state: " << gst_element_state_get_name(state) << std::endl;
        // } else if (ret == GST_STATE_CHANGE_ASYNC) {
        // std::cout << "Pipeline is still changing state asynchronously to: " << gst_element_state_get_name(pending) << std::endl;
        // } else {
        // std::cerr << "Failed to change pipeline state." << std::endl;
        // }

        gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
        pipelineRunning = false;
    }

    debugPrint("exiting stopPipeline()");
}


// private /////////////////////////////////////////

bool GstRecordingPipeline::handleBusMessage(GstBus *bus) {
    bool keepRunning = true;
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if(msg) {
        GError *err;
        gchar *debug_info;
        switch(GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << endl;
                cerr << "Debugging information: " << (debug_info ? debug_info : "none") << endl;
                g_clear_error(&err);
                g_free(debug_info);
                keepRunning = false;
                break;
            case GST_MESSAGE_EOS:
                cerr << "End-Of-Stream reached." << endl;
                keepRunning = false;
                break;
            default:
                cerr << "Unexpected message received." << endl;
                break;
        }
        gst_message_unref(msg);
    }
    return keepRunning;
}

void GstRecordingPipeline::setupNewVideoSink() {
    currentlyRecordingVideoName = make_new_video_name();
    g_object_set(gstData->sink, "location", currentlyRecordingVideoName.c_str(), nullptr);
}

void GstRecordingPipeline::setupGstElements() {
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    #ifdef RPI_MODE
    debugPrint("Creating libcamerasrc source");
    gstData->source = gst_element_factory_make("libcamerasrc", "source");
    #else
    debugPrint("Creating v4l2src source");
    gstData->source = gst_element_factory_make("v4l2src", "source");
    #endif
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    gstData->muxer = gst_element_factory_make("matroskamux", "muxer");
    gstData->sink = gst_element_factory_make("filesink", "sink");

    if(!gstData->pipeline || !gstData->source || !gstData->queue || !gstData->capsfilter || !gstData->videoconvert || !gstData->encoder || !gstData->muxer || !gstData->sink) {
        cerr << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, VIDEO_WIDTH,
        "height", G_TYPE_INT, VIDEO_HEIGHT,
        "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
        nullptr);
    g_object_set(gstData->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL);
    if(!gst_element_link_many(gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL)) {
        std::cerr << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}

std::string GstRecordingPipeline::make_new_video_name() {
    return recordingDir + "/output_" + formatted_time() + ".mkv";

}
