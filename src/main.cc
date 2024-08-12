// // #include "CamRecorder.h"
#include <iostream>
#include <signal.h>

#include "CameraRecorder.h"

using namespace std;

// unique_ptr<CamRecorder> camRecorder;

unique_ptr<CameraRecorder> cameraRecorder;
void catch_sigint(int signum);

int main(int argc, char* argv[]) {
    cameraRecorder = make_unique<CameraRecorder>(argc, argv);

    // camRecorder = make_unique<CamRecorder>();
    signal(SIGINT, catch_sigint);
    
    // camRecorder->recordingLoop();
    // std::atexit([]() {
    //     cameraRecorder->~CameraRecorder();
    // });
    cameraRecorder->mainLoop();
    // std::thread thr(&CameraRecorder::mainLoop, cameraRecorder.get());
    // sleep(5);
    // cameraRecorder->~CameraRecorder();
}

void catch_sigint(int signum)
{
    // cameraRecorder->~CameraRecorder();
    // cameraRecorder->kill();
    cameraRecorder->stopPipeline();
    cout << "Exiting cleanly...\n";

    exit(signum);
}


// #include <gst/gst.h>
// #include <iostream>
// #include <thread>
// #include <chrono>

// void record_segment(GstElement *pipeline, GstElement *filesink, const std::string &location, int duration) {
//     GstBus *bus;
//     GstMessage *msg;

//     // Set the new location for the file sink
//     g_object_set(filesink, "location", location.c_str(), NULL);

//     // Start playing
//     gst_element_set_state(pipeline, GST_STATE_PLAYING);

//     // Wait for the specified duration
//     std::this_thread::sleep_for(std::chrono::seconds(duration));

//     // Send EOS event
//     gst_element_send_event(pipeline, gst_event_new_eos());

//     // Wait until the EOS message is received
//     bus = gst_element_get_bus(pipeline);
//     while (true) {
//         msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
//         if (msg != NULL) {
//             if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
//                 GError *err;
//                 gchar *debug_info;
//                 gst_message_parse_error(msg, &err, &debug_info);
//                 g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
//                 g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
//                 g_clear_error(&err);
//                 g_free(debug_info);
//                 break;
//             } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
//                 g_print("End-Of-Stream reached.\n");
//                 break;
//             }
//             gst_message_unref(msg);
//         }
//     }

//     gst_object_unref(bus);
// }

// int main(int argc, char *argv[]) {
//     GstElement *pipeline, *source, *convert, *encoder, *mux, *filesink;
//     GstBus *bus;
//     GstMessage *msg;

//     gst_init(&argc, &argv);

//     // Create the elements
//     source = gst_element_factory_make("libcamerasrc", "source");
//     convert = gst_element_factory_make("videoconvert", "convert");
//     encoder = gst_element_factory_make("x264enc", "encoder");
//     mux = gst_element_factory_make("mp4mux", "mux");
//     filesink = gst_element_factory_make("filesink", "filesink");

//     if (!source || !convert || !encoder || !mux || !filesink) {
//         g_printerr("Not all elements could be created.\n");
//         return -1;
//     }

//     // Set properties for the source element if needed
//     // g_object_set(source, "property-name", value, NULL);

//     // Create the empty pipeline
//     pipeline = gst_pipeline_new("test-pipeline");

//     if (!pipeline) {
//         g_printerr("Pipeline could not be created.\n");
//         return -1;
//     }

//     // Build the pipeline
//     gst_bin_add_many(GST_BIN(pipeline), source, convert, encoder, mux, filesink, NULL);
//     if (!gst_element_link_many(source, convert, encoder, mux, filesink, NULL)) {
//         g_printerr("Elements could not be linked.\n");
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Record the first segment
//     record_segment(pipeline, filesink, "output1.mp4", 5);

//     // Set the pipeline state to NULL and handle cleanup
//     std::cout << "Setting pipeline state to NULL\n";
//     gst_element_set_state(pipeline, GST_STATE_NULL);

//     // Handle any pending messages
//     bus = gst_element_get_bus(pipeline);
//     while ((msg = gst_bus_pop(bus))) {
//         gst_message_unref(msg);
//     }
//     gst_object_unref(bus);

//     // Record the second segment
//     record_segment(pipeline, filesink, "output2.mp4", 5);

//     // Clean up
//     gst_element_set_state(pipeline, GST_STATE_NULL);

//     // Handle any remaining messages
//     bus = gst_element_get_bus(pipeline);
//     while ((msg = gst_bus_pop(bus))) {
//         gst_message_unref(msg);
//     }
//     gst_object_unref(bus);

//     gst_object_unref(pipeline);

//     return 0;
// }
