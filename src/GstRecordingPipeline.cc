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

void GstRecordingPipeline::pipelineRunner() {
    debugPrint("startPipeline()");

    if(!pipelineRunning) {
        debugPrint("Starting pipeline");

        setupNewVideoSink();
        gst_element_set_state(gstData->pipeline, GST_STATE_PLAYING);
        pipelineRunning = true;

        gstData->bus = gst_element_get_bus(gstData->pipeline);
        while(handleBusMessage(gstData->bus)) {}

        gst_object_unref(gstData->bus);
        pipelineRunning = false;
    }

    debugPrint("exiting startPipeline()");
}

void GstRecordingPipeline::startPipeline() {
    pipelineThread = std::thread(&GstRecordingPipeline::pipelineRunner, this);
    
}

void GstRecordingPipeline::stopPipeline() {
    debugPrint("stopPipeline()");

    if(pipelineRunning) {
        debugPrint("Stopping pipeline");
        
        debugPrint("Sending EOS event");
        gst_element_send_event(gstData->pipeline, gst_event_new_eos());
        pipelineThread.join();
        // while(pipelineRunning) {std::this_thread::sleep_for(std::chrono::milliseconds(100));}
        debugPrint("Setting pipeline state to NULL");
        gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
    }

    debugPrint("exiting stopPipeline()");
}


// private /////////////////////////////////////////

bool GstRecordingPipeline::handleBusMessage(GstBus *bus) {
    bool keepRunning = true;
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 100*GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
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
    cout << "Recording to: " << currentlyRecordingVideoName << endl;
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
