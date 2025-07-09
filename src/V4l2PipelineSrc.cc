#include "V4l2PipelineSrc.h"

void V4l2PipelineSrc::wait_for_video_device() {
    const std::string devicePath = "/dev/video0";

    while (!std::filesystem::exists(devicePath)) {
        std::cout << "Waiting for " << devicePath << " to become available..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << devicePath << " is now available." << std::endl;
}


void V4l2PipelineSrc::setupV4l2SrcAndTee() {
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