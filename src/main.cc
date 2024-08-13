#include <iostream>
// #include <signal.h>
// #include <memory>

// // #include "GstRecordingPipeline.h"
// // #include "CamService.h"
// #include "RecordingPipeline.h"
// using namespace std;

// // unique_ptr<CamService> camservice;
// // unique_ptr<GstRecordingPipeline> gstRecordingPipeline;
// unique_ptr<RecordingPipeline> recordingPipeline;
// void catch_signal(int signum);

// int main(int argc, char* argv[]) {
//     // camservice = make_unique<CamService>( &argc, &argv);
//     recordingPipeline = make_unique<RecordingPipeline>("./recordings", 300, &argc, &argv);
//     signal(SIGINT, catch_signal);
//     signal(SIGTERM, catch_signal); // Termination request
//     signal(SIGQUIT, catch_signal); // Ctrl+\ (Quit)
//     signal(SIGHUP, catch_signal);  // Terminal closed
    
//     // camservice->mainLoop();
//     cout << "Starting pipeline" << endl;
//     recordingPipeline->startPipeline();
//     // std::this_thread::sleep_for(std::chrono::seconds(7));
//     // recordingPipeline->createNewVideo();
//     // std::this_thread::sleep_for(std::chrono::seconds(300));
//     // recordingPipeline->stopPipeline();
//     while(true) {
//         string input;
//         cin >> input;
//         if (input.find('n') != string::npos) {
//             cout << "Creating new video" << endl;
//             recordingPipeline->createNewVideo();
//         } else if (input.find('q') != string::npos) {
//             cout << "Stopping pipeline" << endl;
//             recordingPipeline->stopPipeline();
//             break;
//         }
//     }
// }

// void catch_signal(int signum)
// {
//     cout << "Exiting cleanly. Received signal " << strsignal(signum) << endl;
//     recordingPipeline->stopPipeline();
//     exit(signum);
// }

#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // Create GStreamer elements
    GstElement *pipeline = gst_pipeline_new("video-pipeline");
    GstElement *source = gst_element_factory_make("libcamerasrc", "source");
    GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    GstElement *encoder = gst_element_factory_make("v4l2h264enc", "encoder");
    GstElement *muxer = gst_element_factory_make("mp4mux", "muxer");
    GstElement *sink = gst_element_factory_make("filesink", "sink");

    if (!pipeline || !source || !videoconvert || !encoder || !muxer || !sink) {
        std::cerr << "Failed to create elements" << std::endl;
        return -1;
    }

    // Set properties
    g_object_set(sink, "location", "output.mp4", NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, encoder, muxer, sink, NULL);
    if (!gst_element_link_many(source, videoconvert, encoder, muxer, sink, NULL)) {
        std::cerr << "Failed to link elements" << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != nullptr) {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
                std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                std::cout << "End-Of-Stream reached." << std::endl;
                break;
            default:
                break;
        }
        gst_message_unref(msg);
    }

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    return 0;
}
