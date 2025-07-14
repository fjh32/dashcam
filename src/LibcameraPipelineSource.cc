#include "LibcameraPipelineSource.h"

GstPad* LibcameraPipelineSource::getSourcePad() {
    return gst_element_get_static_pad(tee, "src");
}

GstElement* LibcameraPipelineSource::getTee() {
    return tee;
}

void LibcameraPipelineSource::setupSource(GstElement* pipeline) {
    std::cout << "Creating gstreamer libcamera source\n";
    this->source = gst_element_factory_make("libcamerasrc", "source");
    this->encoder = gst_element_factory_make("x264enc", "encoder");
    this->queue = gst_element_factory_make("queue", "queue");
    this->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    this->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    
    this->videoflip = gst_element_factory_make("videoflip", "videoflip");

    this->parser = gst_element_factory_make("h264parse", "h264parser");
    this->tee = gst_element_factory_make ("tee", "tee");

    if(!pipeline || 
        !this->source || 
        !this->queue || 
        !this->capsfilter || 
        !this->videoconvert || 
        !this->videoflip ||
        !this->encoder || 
        !this->parser || 
        !this->tee) {
        cout << "Failed to create elements\n";
        exit(1); // TODO: throw exception instead
    }

    // g_object_set(G_OBJECT(this->source),
    // "format", GST_VIDEO_FORMAT_NV12,  // This sets 4:2:0 format
    // NULL);
    
    g_object_set(G_OBJECT(this->encoder),
        "tune", 0x00000004,  // zerolatency
        "speed-preset", 1,   // ultrafast
        "bitrate", 2000,     // Adjust based on your needs
        "key-int-max", FRAME_RATE,  // GOP size, adjust if needed
        // "profile", 2,        // Change to main profile (2) instead of baseline (1)
        // "level", 31,         // Set to Level 3.1 (compatible with most devices)
        NULL);

    g_object_set(this->videoflip, "method", 2, NULL); // rotate-180

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "NV12",
        // "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, VIDEO_WIDTH,
        "height", G_TYPE_INT, VIDEO_HEIGHT,
        "framerate", GST_TYPE_FRACTION, FRAME_RATE, 1,
        nullptr);
    g_object_set(this->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // g_object_set(G_OBJECT(this->parser),
    //             "config-interval", -1,
    //             NULL);

    gst_bin_add_many(GST_BIN(pipeline), 
                                this->source, 
                                this->queue, 
                                this->capsfilter, 
                                this->videoflip,      
                                this->videoconvert, 
                                this->encoder,  
                                this->parser,
                                this->tee, 
                                NULL);
    if (!gst_element_link_many(this->source, 
                                this->queue, 
                                this->capsfilter, 
                                this->videoflip,
                                this->videoconvert, 
                                this->encoder, 
                                this->parser, 
                                this->tee, NULL)) {
        std::cout << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}