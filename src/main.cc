#include <iostream>
#include <signal.h>
#include <memory>

// #include "GstRecordingPipeline.h"
// #include "CamService.h"
#include "RecordingPipeline.h"
using namespace std;

// unique_ptr<CamService> camservice;
// unique_ptr<GstRecordingPipeline> gstRecordingPipeline;
unique_ptr<RecordingPipeline> recordingPipeline;
void catch_signal(int signum);

int main(int argc, char* argv[]) {
    // camservice = make_unique<CamService>( &argc, &argv);
    recordingPipeline = make_unique<RecordingPipeline>("./recordings", 300, &argc, &argv);
    signal(SIGINT, catch_signal);
    signal(SIGTERM, catch_signal); // Termination request
    signal(SIGQUIT, catch_signal); // Ctrl+\ (Quit)
    signal(SIGHUP, catch_signal);  // Terminal closed
    
    // camservice->mainLoop();
    cout << "Starting pipeline" << endl;
    recordingPipeline->startPipeline();
    // std::this_thread::sleep_for(std::chrono::seconds(7));
    // recordingPipeline->createNewVideo();
    // std::this_thread::sleep_for(std::chrono::seconds(300));
    // recordingPipeline->stopPipeline();
    while(true) {
        string input;
        cin >> input;
        if (input.find('n') != string::npos) {
            cout << "Creating new video" << endl;
            recordingPipeline->createNewVideo();
        } else if (input.find('q') != string::npos) {
            cout << "Stopping pipeline" << endl;
            recordingPipeline->stopPipeline();
            break;
        }
    }
}

void catch_signal(int signum)
{
    cout << "Exiting cleanly. Received signal " << strsignal(signum) << endl;
    recordingPipeline->stopPipeline();
    exit(signum);
}
// #include <gst/gst.h>
// #include <iostream>

// int main(int argc, char *argv[]) {
//     gst_init(&argc, &argv);

//     // Create the pipeline
//     GstElement *pipeline = gst_pipeline_new("pipeline");

//     // Create the elements
//     GstElement *source = gst_element_factory_make("v4l2src", "source");
//     GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
//     GstElement *encoder = gst_element_factory_make("x264enc", "encoder");
//     GstElement *muxer = gst_element_factory_make("mp4mux", "muxer");
//     GstElement *splitmuxsink = gst_element_factory_make("splitmuxsink", "sink");

//     if (!pipeline || !source || !videoconvert || !encoder || !muxer || !splitmuxsink) {
//         std::cerr << "Failed to create elements" << std::endl;
//         return -1;
//     }

//     // Set properties
//     g_object_set(source, "device", "/dev/video0", NULL);  // Adjust as needed
//     g_object_set(splitmuxsink, "muxer", muxer, NULL);
//     g_object_set(splitmuxsink, "location", "./output%05d.mp4", NULL);
//     g_object_set(splitmuxsink, "max-size-time", (guint64)600 * GST_SECOND, NULL);

//     // Add elements to the pipeline
//     gst_bin_add_many(GST_BIN(pipeline), source, videoconvert, encoder, splitmuxsink, NULL);

//     // Link the elements together
//     if (!gst_element_link_many(source, videoconvert, encoder, splitmuxsink, NULL)) {
//         std::cerr << "Failed to link elements" << std::endl;
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Start playing the pipeline
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     // Wait until error or EOS
//     GstBus *bus = gst_element_get_bus(pipeline);
//     GstMessage *msg;
//     bool running = true;
//     while (running) {
//         msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
//         if (msg != NULL) {
//             switch (GST_MESSAGE_TYPE(msg)) {
//                 case GST_MESSAGE_ERROR: {
//                     GError *err;
//                     gchar *debug_info;
//                     gst_message_parse_error(msg, &err, &debug_info);
//                     std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
//                     std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
//                     g_clear_error(&err);
//                     g_free(debug_info);
//                     running = false;
//                     break;
//                 }
//                 case GST_MESSAGE_EOS:
//                     std::cout << "End-Of-Stream reached." << std::endl;
//                     running = false;
//                     break;
//                 default:
//                     break;
//             }
//             gst_message_unref(msg);
//         }
//     }

//     // Clean up
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(pipeline);
//     gst_object_unref(bus);

//     return 0;
// }