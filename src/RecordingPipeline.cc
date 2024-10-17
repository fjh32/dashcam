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

void RecordingPipeline::setupSoftwareEncodingRecorder() {
    debugPrint("Setting up SOFTWARE ENCODING pipeline");
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    #ifdef RPI_MODE
    debugPrint("Creating libcamerasrc source");
    gstData->source = gst_element_factory_make("libcamerasrc", "source");
    #else
    debugPrint("Creating v4l2src source");
    gstData->source = gst_element_factory_make("v4l2src", "source");
    #endif

    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    gstData->h264parser = gst_element_factory_make("h264parse", "h264parser");
    gstData->tee = gst_element_factory_make ("tee", "tee");

    if(!gstData->pipeline || 
        !gstData->source || 
        !gstData->queue || 
        !gstData->capsfilter || 
        !gstData->videoconvert || 
        !gstData->encoder || 
        !gstData->h264parser || 
        !gstData->tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }

    // g_object_set(G_OBJECT(gstData->source),
    // "format", GST_VIDEO_FORMAT_NV12,  // This sets 4:2:0 format
    // NULL);
    
    g_object_set(G_OBJECT(gstData->encoder),
        "tune", 0x00000004,  // zerolatency
        "speed-preset", 1,   // ultrafast
        "bitrate", 2000,     // Adjust based on your needs
        "key-int-max", FRAME_RATE,  // GOP size, adjust if needed
        "profile", 2,        // Change to main profile (2) instead of baseline (1)
        "level", 31,         // Set to Level 3.1 (compatible with most devices)
        NULL);

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "NV12",
        // "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, VIDEO_WIDTH,
        "height", G_TYPE_INT, VIDEO_HEIGHT,
        "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
        nullptr);
    g_object_set(gstData->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(gstData->pipeline), 
                            gstData->source, 
                            gstData->queue, 
                            gstData->capsfilter, 
                            gstData->videoconvert, 
                            gstData->encoder,  
                            gstData->h264parser,
                            gstData->tee, 
                            NULL);
    if(!gst_element_link_many(gstData->source, 
                                gstData->queue, 
                                gstData->capsfilter, 
                                gstData->videoconvert, 
                                gstData->encoder, 
                                gstData->h264parser, 
                                gstData->tee, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}

void RecordingPipeline::setupHardwareEncodingRecorder() {
    debugPrint("Setting up RPI ZERO HARDWARE ENCODING pipeline");

    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    gstData->source = gst_element_factory_make("libcamerasrc", "source");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("v4l2convert", "videoconvert");
    gstData->encoder = gst_element_factory_make("v4l2h264enc", "encoder");
    gstData->post_encode_caps = gst_element_factory_make("capsfilter", "post_encode_caps");
    gstData->h264parser = gst_element_factory_make("h264parse", "h264parser");
    gstData->tee = gst_element_factory_make ("tee", "tee");

    if(!gstData->pipeline || !gstData->source || !gstData->capsfilter || !gstData->videoconvert || !gstData->encoder || !gstData->post_encode_caps || !gstData->h264parser || !gstData->tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }

    ///////////// Encoder properties
    GstStructure *extra_controls = gst_structure_from_string("controls,repeat_sequence_header=1", NULL);
    if (extra_controls) {
        g_object_set(G_OBJECT(gstData->encoder), "extra-controls", extra_controls, NULL);
        gst_structure_free(extra_controls);
    } else {
        g_printerr("Failed to set extra-controls on v4l2h264enc.");
    }
    /////////////////////////////////

    //////////// Caps filters
    g_object_set(G_OBJECT(gstData->capsfilter), "caps", gst_caps_from_string("video/x-raw,width=1280,height=720,format=NV12"), NULL);

    g_object_set(G_OBJECT(gstData->post_encode_caps), "caps", gst_caps_from_string("video/x-h264,level=(string)4"), NULL);
    /////////////////////////////////

    gst_bin_add_many(GST_BIN(gstData->pipeline), 
                            gstData->source, 
                            gstData->capsfilter, 
                            gstData->videoconvert, 
                            gstData->encoder,
                            gstData->post_encode_caps,
                            gstData->h264parser, 
                            gstData->tee, 
                            NULL);

    bool link_status = gst_element_link_many(gstData->source, 
                                                gstData->capsfilter, 
                                                gstData->videoconvert, 
                                                gstData->encoder, 
                                                gstData->post_encode_caps, 
                                                gstData->h264parser, 
                                                gstData->tee, NULL);

    if (!link_status) {
        std::cout << "Error: Could not link gstreamer elements in RPI ZERO PIPELINE." << std::endl;
        exit(1);
    }
}

void RecordingPipeline::setupGstElements() {
    #ifndef RPI_ZERO_MODE
    setupSoftwareEncodingRecorder();
    #else
    setupHardwareEncodingRecorder();
    #endif

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
    gstData->hlsmux = gst_element_factory_make("mpegtsmux", "hlsmux");
    gstData->hlssink = gst_element_factory_make("hlssink", "hlssink");

    if (!gstData->hls_queue 
        || !gstData->h264parse
        || !gstData->hlsmux 
        || !gstData->hlssink) {
        std::cout << "Error: Failed to create HLS elements." << std::endl;
        exit(1);
    }

    if (!std::filesystem::exists(HLS_FILE_ROOT)) {
        std::filesystem::create_directory(HLS_FILE_ROOT);
    }
    std::string livestream_location = recordingDir + "/" + "livestream.m3u8";
    std::string segment_location = recordingDir + "/" + "segment%05d.ts";

    // config interval -1 means no config frame and IS NECESSARY
    g_object_set(G_OBJECT(gstData->h264parse),
                "config-interval", -1,
                // "output-format", 2,
                NULL);

    g_object_set(G_OBJECT(gstData->hlssink),
                "playlist-location", livestream_location.c_str(),
                "location", segment_location.c_str(),
                "target-duration", 4,
                "playlist-length", 5,
                "max-files", 10,
                // "playlist-root", "https://ripplein.space/",
                "playlist-root", "http://192.168.1.71:8888/",
                "dynamic", TRUE,
                "disable-hls-cache", TRUE,
                "program-date-time", TRUE,
                 NULL);

    

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(gstData->pipeline),
                    gstData->hls_queue,
                    gstData->h264parse,
                    gstData->hlsmux,    
                    gstData->hlssink,
                    NULL);

    // Link the elements
    if (!gst_element_link_many(gstData->hls_queue, 
                                gstData->h264parse,
                                gstData->hlsmux, 
                                gstData->hlssink, NULL)) {
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
