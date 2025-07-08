#include "RecordingPipeline.h"


using namespace std;

void handle_error_naive(std::string errMsg);
void wait_for_video_device();

// callback function to create new filename for splitmuxsink
static gchar* make_new_filename(GstElement *splitmux, guint fragment_id, gpointer user_data) {
    RecordingPipeline* instance = static_cast<RecordingPipeline*>(user_data);

    // Create filename for .ts
    // string ts_filename = "output_" + formatted_time() + ".ts";
    string ts_filename = "output_" + formatted_time() + ".ts";
    string ts_filepath = instance->recordingDir + "/" + ts_filename;
    instance->currentlyRecordingVideoName = ts_filepath;

    cout << "Recording new video: " << ts_filepath << endl;

    // Create matching .m3u8 playlist file
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

    return g_strdup(ts_filepath.c_str()); // GStreamer will free this string
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

        #if !defined(RPI_MODE) && !defined(RPI_ZERO_MODE)
        wait_for_video_device();
        #endif

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
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 500*GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_ELEMENT | GST_MESSAGE_EOS));
    if(msg) {
        GError *err;
        gchar *debug_info;
        switch(GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ELEMENT: {
                cout << "Element message received." << endl;
                const GstStructure *s = gst_message_get_structure(msg);
                if (s && gst_structure_has_name(s, "splitmuxsink-fragment-closed")) {
                    // You can safely access fields now
                }
                break;
            }
            case GST_MESSAGE_ERROR: {
                gst_message_parse_error(msg, &err, &debug_info);
                std::string errMsg = err ? err->message : "";
                std::string dbgInfo = debug_info ? debug_info : "";

                cout << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << errMsg << endl;
                cout << "Debugging information: " << (debug_info ? debug_info : "none") << endl;

                handle_error_naive(errMsg);
                handle_error_naive(dbgInfo);

                g_clear_error(&err);
                g_free(debug_info);
                keepRunning = false;
                break;
            }
            case GST_MESSAGE_EOS: {
                cout << "End-Of-Stream reached." << endl;
                keepRunning = false;
                break;
            }
            default: {
                cout << "Unexpected message received." << endl;
                break;
            }
        }
        gst_message_unref(msg);
    }
    return keepRunning;
}

void handle_error_naive(std::string errMsg) {
    // Throw if error message contains "/dev/video"
    if (errMsg.find("/dev/video") != std::string::npos) {
        std::string fullMsg = "Fatal device error: " + errMsg;
        throw std::runtime_error(fullMsg);
    }
}

void RecordingPipeline::setupGstElements() {
    #ifdef RPI_MODE
    setupSoftwareEncodingRecorder();
    #elif RPI_ZERO_MODE
    setupHardwareEncodingRecorder();
    #else
    setupV4l2Recording();
    #endif

    setupFileSinkElements();
    setupHlsElements();

    cout << "Gstreamer elements setup successfully." << endl;
}


void RecordingPipeline::setupV4l2Recording() {
    std::cout << "Setting up V4L2 pipeline\n";
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    gstData->source = gst_element_factory_make("v4l2src", "source");
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    gstData->h264parser = gst_element_factory_make("h264parse", "h264parser");
    gstData->tee = gst_element_factory_make ("tee", "tee");

    if(!gstData->pipeline || 
        !gstData->source || 
        !gstData->videoconvert || 
        !gstData->capsfilter || 
        !gstData->encoder || 
        !gstData->h264parser || 
        !gstData->tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }
    g_object_set(gstData->source, "device", "/dev/video0", nullptr);

    // the caps of my webcam...
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 640,
    "height", G_TYPE_INT, 480,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL);
    g_object_set(G_OBJECT(gstData->capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);
    
    // // Stream speed goes up, quality goes down
    // g_object_set(G_OBJECT(gstData->encoder),
    // "tune", 0x00000004,  // zerolatency
    // "speed-preset", 1,   // ultrafast
    // "bitrate", 2000,     // Adjust based on your needs
    // "key-int-max", FRAME_RATE,  // GOP size, adjust if needed
    // // "profile", 2,        // Change to main profile (2) instead of baseline (1)
    // // "level", 31,         // Set to Level 3.1 (compatible with most devices)
    // NULL);

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
                                gstData->tee, 
                                NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}

void RecordingPipeline::setupSoftwareEncodingRecorder() {
    std::cout << "Setting up SOFTWARE ENCODING pipeline\n";
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    #ifdef RPI_MODE
    std::cout << "Creating libcamerasrc source\n";
    gstData->source = gst_element_factory_make("libcamerasrc", "source");
    #else
    std::cout << "Creating v4l2src source\n";
    gstData->source = gst_element_factory_make("v4l2src", "source");
    #endif
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    gstData->queue = gst_element_factory_make("queue", "queue");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    
    gstData->videoflip = gst_element_factory_make("videoflip", "videoflip");

    gstData->h264parser = gst_element_factory_make("h264parse", "h264parser");
    gstData->tee = gst_element_factory_make ("tee", "tee");

    if(!gstData->pipeline || 
        !gstData->source || 
        !gstData->queue || 
        !gstData->capsfilter || 
        !gstData->videoconvert || 
        !gstData->videoflip ||
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
        // "profile", 2,        // Change to main profile (2) instead of baseline (1)
        // "level", 31,         // Set to Level 3.1 (compatible with most devices)
        NULL);

    g_object_set(gstData->videoflip, "method", 2, NULL); // rotate-180

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

    // g_object_set(G_OBJECT(gstData->h264parser),
    //             "config-interval", -1,
    //             NULL);

    gst_bin_add_many(GST_BIN(gstData->pipeline), 
                                gstData->source, 
                                gstData->queue, 
                                gstData->capsfilter, 
                                gstData->videoflip,      
                                gstData->videoconvert, 
                                gstData->encoder,  
                                gstData->h264parser,
                                gstData->tee, 
                                NULL);
    if (!gst_element_link_many(gstData->source, 
                                gstData->queue, 
                                gstData->capsfilter, 
                                gstData->videoflip,
                                gstData->videoconvert, 
                                gstData->encoder, 
                                gstData->h264parser, 
                                gstData->tee, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}



void RecordingPipeline::setupFileSinkElements() {
    gstData->file_sink_queue = gst_element_factory_make("queue", "file_sink_queue");
    if (!gstData->file_sink_queue) {
        std::cerr << "Error: Failed to queue in setupFileSinkElements()." << std::endl;
        exit(1);
    }
    // gstData->muxer = gst_element_factory_make("mp4mux", "muxer");
    gstData->muxer = gst_element_factory_make("mpegtsmux", "muxer");  // MPEG-TS muxer
    if (!gstData->muxer) {
        std::cerr << "Error: Could not create 'mpegtsmux'. Plugin missing or GStreamer misconfigured?" << std::endl;
        exit(1);
    }

    gstData->sink = gst_element_factory_make("splitmuxsink", "sink");
    if ( !gstData->sink ) {
        std::cerr << "Error: Failed to create splitmuxsink." << std::endl;
        exit(1);
    }
    g_object_set(gstData->sink, "muxer", gstData->muxer, NULL);
    g_object_set(gstData->sink, "max-size-time", (guint64)video_duration * GST_SECOND, NULL); // 30 minutes
    g_signal_connect(gstData->sink, "format-location", G_CALLBACK(make_new_filename), this);

    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->file_sink_queue, gstData->sink, NULL);
    if(!gst_element_link_many(gstData->file_sink_queue, gstData->sink, NULL)) {
        std::cout << "Error: Could not link gstreamer filesink elements." << std::endl;
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
    std::cout << "File sink elements setup successfully.\n";
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

    // config interval -1 means no config frame and IS NECESSARY
    // cannot be set to 0
    g_object_set(G_OBJECT(gstData->h264parse),
                "config-interval", 1,
                NULL);

    std::string livestream_location = recordingDir + "/" + "livestream.m3u8";
    std::string segment_location = recordingDir + "/" + "segment%05d.ts";

    // this->webroot = "http://" + get_ip_address();
    this->webroot = "/recordings/";
    #ifdef DEBUG
    this->webroot = this->webroot + ":8888";
    #endif
    g_object_set(G_OBJECT(gstData->hlssink),
                "playlist-location", livestream_location.c_str(),
                "location", segment_location.c_str(),
                "target-duration", 1,
                "playlist-length", 2,
                "max-files", 2,
                "playlist-root", this->webroot.c_str(),
		 //                "disable-hls-cache", TRUE,
                // "program-date-time", TRUE,
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

    std::cout << "HLS elements setup successfully. Web root " << webroot << std::endl;
}

void RecordingPipeline::ffmpeg_faststart(std::string filepath) {
    try{
        debugPrint("Running ffmpeg faststart on: " + filepath);
        std::string prefix = "output_";
        std::string newfilename = filepath;
        newfilename = newfilename.replace(newfilename.find(prefix), prefix.length(), ""); // Remove prefix
        std::string command = "ffmpeg -y -i " + filepath + " -movflags faststart -c copy " + newfilename + " > /dev/null 2>&1";
        std::cout << "Executing ffmpeg faststart command: " << command << std::endl;
        if (std::system(command.c_str()) != 0) {
            std::cerr << "Error executing ffmpeg command for " << filepath << std::endl;
        }
        std::system(("rm " + filepath).c_str()); // Remove the original file after processing
        // debugPrint("ffmpeg faststart completed for: " + newfilename);
    } catch (const std::exception& e) {
        std::cerr << "Error during ffmpeg faststart on " << filepath << ": " << e.what() << std::endl;
    }
}

void RecordingPipeline::ffmpeg_faststart_thread(std::string filename) {
    // ffmpegThread = nullptr;
    ffmpegThread = make_shared<std::thread>([this, filename]() {
        ffmpeg_faststart(filename);
    });
    ffmpegThread->detach(); // Detach the thread to allow it to run independently
}





void RecordingPipeline::setupHardwareEncodingRecorder() {
    std::cout << "Setting up RPI ZERO HARDWARE ENCODING pipeline\n";

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


void wait_for_video_device() {
    const std::string devicePath = "/dev/video0";

    while (!std::filesystem::exists(devicePath)) {
        std::cout << "Waiting for " << devicePath << " to become available..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << devicePath << " is now available." << std::endl;
}