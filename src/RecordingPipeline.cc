#include "RecordingPipeline.h"


using namespace std;

void handle_error_naive(std::string errMsg);


/// RecordingPipeline class
RecordingPipeline::RecordingPipeline( const char dir[], int vid_duration, int* argc, char*** argv) 
                            : recordingDir(dir), video_duration(vid_duration), pipelineRunning(false), currentlyRecordingVideoName("None") {
    debugPrint("Creating RecordingPipeline object");
    this->video_duration = this->video_duration < 2 ? 2 : this->video_duration;
    gst_init(argc, argv);

    makeDir(recordingDir.c_str());
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
    gst_object_unref(this->pipeline);
}

void RecordingPipeline::setSource(std::unique_ptr<PipelineSource> src) {
    source = std::move(src);
}

void RecordingPipeline::addSink(std::unique_ptr<PipelineSink> sink) {
    sinks.push_back(std::move(sink));
}

GstElement* RecordingPipeline::getSourceTee() {
    return source ? source->getTee() : nullptr;
}

void RecordingPipeline::buildPipeline() {
    this->pipeline = gst_pipeline_new("recording_pipeline");
    if (!this->pipeline) throw std::runtime_error("Failed to create pipeline");

    if (!source) throw std::runtime_error("No source set");
    if (sinks.empty()) throw std::runtime_error("No sinks added");

    source->setupSource(this->pipeline);

    for (auto& sink : sinks) {
        sink->setupSink(this->pipeline);
    }
}

void RecordingPipeline::pipelineRunner() {
    debugPrint("startPipeline() at " + formatted_time());

    if(!pipelineRunning) {
        this->buildPipeline();

        debugPrint("Starting pipeline");
        gst_element_set_state(this->pipeline, GST_STATE_PLAYING);
        pipelineRunning = true;
        this->bus = gst_element_get_bus(this->pipeline);
        while(handleBusMessage(this->bus)) {}
        gst_object_unref(this->bus);

        pipelineRunning = false;
    }

    debugPrint("exiting startPipeline()");
}

void RecordingPipeline::startPipeline() {
    pipelineThread = std::thread(&RecordingPipeline::pipelineRunner, this);
}

void RecordingPipeline::createNewVideo() {
    debugPrint("Received createNewVideo() signal.");
    for(const auto& sink : this->sinks) {
        g_signal_emit_by_name (sink->getSinkElement(), "split-now");
    }
}

void RecordingPipeline::stopPipeline() {
    debugPrint("stopPipeline()");

    if(pipelineRunning) {
        debugPrint("Sending EOS event");
        gst_element_send_event(this->pipeline, gst_event_new_eos());
        pipelineThread.join();
        debugPrint("Setting pipeline state to NULL");
        gst_element_set_state(this->pipeline, GST_STATE_NULL);
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

// void RecordingPipeline::setupHardwareEncodingRecorder() {
//     std::cout << "Setting up RPI ZERO HARDWARE ENCODING pipeline\n";

//     this->pipeline = gst_pipeline_new("recording_pipeline");

//     gstData->source = gst_element_factory_make("libcamerasrc", "source");
//     gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
//     gstData->videoconvert = gst_element_factory_make("v4l2convert", "videoconvert");
//     gstData->encoder = gst_element_factory_make("v4l2h264enc", "encoder");
//     gstData->post_encode_caps = gst_element_factory_make("capsfilter", "post_encode_caps");
//     gstData->h264parser = gst_element_factory_make("h264parse", "h264parser");
//     gstData->tee = gst_element_factory_make ("tee", "tee");

//     if(!this->pipeline || !gstData->source || !gstData->capsfilter || !gstData->videoconvert || !gstData->encoder || !gstData->post_encode_caps || !gstData->h264parser || !gstData->tee) {
//         cout << "Failed to create elements\n";
//         exit(1); // TODO: throw exception instead
//     }

//     ///////////// Encoder properties
//     GstStructure *extra_controls = gst_structure_from_string("controls,repeat_sequence_header=1", NULL);
//     if (extra_controls) {
//         g_object_set(G_OBJECT(gstData->encoder), "extra-controls", extra_controls, NULL);
//         gst_structure_free(extra_controls);
//     } else {
//         g_printerr("Failed to set extra-controls on v4l2h264enc.");
//     }
//     /////////////////////////////////

//     //////////// Caps filters
//     g_object_set(G_OBJECT(gstData->capsfilter), "caps", gst_caps_from_string("video/x-raw,width=1280,height=720,format=NV12"), NULL);

//     g_object_set(G_OBJECT(gstData->post_encode_caps), "caps", gst_caps_from_string("video/x-h264,level=(string)4"), NULL);
//     /////////////////////////////////

//     gst_bin_add_many(GST_BIN(this->pipeline), 
//                             gstData->source, 
//                             gstData->capsfilter, 
//                             gstData->videoconvert, 
//                             gstData->encoder,
//                             gstData->post_encode_caps,
//                             gstData->h264parser, 
//                             gstData->tee, 
//                             NULL);

//     bool link_status = gst_element_link_many(gstData->source, 
//                                                 gstData->capsfilter, 
//                                                 gstData->videoconvert, 
//                                                 gstData->encoder, 
//                                                 gstData->post_encode_caps, 
//                                                 gstData->h264parser, 
//                                                 gstData->tee, NULL);

//     if (!link_status) {
//         std::cout << "Error: Could not link gstreamer elements in RPI ZERO PIPELINE." << std::endl;
//         exit(1);
//     }
// }


