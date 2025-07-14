#include "V4l2PipelineSource.h"

void V4l2PipelineSource::wait_for_video_device() {
    const std::string devicePath = "/dev/video0";

    while (!std::filesystem::exists(devicePath)) {
        std::cout << "Waiting for " << devicePath << " to become available..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << devicePath << " is now available." << std::endl;
}

GstPad* V4l2PipelineSource::getSourcePad() {
    return gst_element_get_static_pad(tee, "src");
}

GstElement* V4l2PipelineSource::getTee() {
    return tee;
}

void V4l2PipelineSource::setupSource(GstElement* pipeline) {
    wait_for_video_device();

    source = gst_element_factory_make("v4l2src", "source");
    queue = gst_element_factory_make("queue", "queue");
    capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    nv12_capsfilter = gst_element_factory_make("capsfilter", "nv12_capsfilter");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    encoder = gst_element_factory_make("x264enc", "encoder");
    parser = gst_element_factory_make("h264parse", "parser");
    tee = gst_element_factory_make ("tee", "tee");

    if(!pipeline || 
        !source || 
        !videoconvert || 
        !capsfilter || 
        !nv12_capsfilter ||
        !encoder || 
        !parser || 
        !tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }
    g_object_set(source, "device", "/dev/video0", nullptr);

    // the caps of my webcam...
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 640,
        "height", G_TYPE_INT, 480,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL);
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    GstCaps* nv12_caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "NV12",
        NULL);
    g_object_set(G_OBJECT(nv12_capsfilter), "caps", nv12_caps, NULL);
    gst_caps_unref(nv12_caps);

    gst_bin_add_many(GST_BIN(pipeline),
                     source,
                     queue,
                     capsfilter,
                     videoconvert,
                     nv12_capsfilter,
                     encoder,
                     parser,
                     tee,
                     NULL);
    
    if(!gst_element_link_many(source,
                                queue,
                                capsfilter,
                                videoconvert,
                                nv12_capsfilter,
                                encoder, 
                                parser, 
                                tee, 
                                NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}