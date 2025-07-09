#include "LibcameraHlsPipeline.h"

LibcameraHlsPipeline::LibcameraHlsPipeline(const char* dir, int vid_duration, int* argc, char*** argv)
    : RecordingPipeline(dir, vid_duration, argc, argv) {
    setupRecordingPipeline();
}

void LibcameraHlsPipeline::setupRecordingPipeline() {
    setupLibcameraRecording();
    setupFileSinkElements();
    setupHlsElements();

    cout << "Gstreamer elements setup successfully." << endl;
}

void LibcameraHlsPipeline::setupLibcameraRecording() {
    std::cout << "Setting up LIBCAMERA SOFTWARE ENCODING pipeline\n";
    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    std::cout << "Creating libcamerasrc source\n";
    gstData->source = gst_element_factory_make("libcamerasrc", "source");
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