#include "HlsPipelineSink.h"

HlsPipelineSink::HlsPipelineSink(RecordingPipeline* context) 
: context(context) { }

GstPad* HlsPipelineSink::getSinkPad() {
    return gst_element_get_static_pad(queue, "sink");
}

GstElement* HlsPipelineSink::getSinkElement() {
    return sink;
}

void HlsPipelineSink::setupSink(GstElement* pipeline) {
    std::cout << "Creating HlsPipelineSink" << std::endl;
    this->queue = gst_element_factory_make("queue", "hls_queue");
    this->parser = gst_element_factory_make("h264parse", "h264parse");
    this->mux = gst_element_factory_make("mpegtsmux", "hlsmux");
    this->sink = gst_element_factory_make("hlssink", "hlssink");

    if (!this->queue 
        || !this->parser
        || !this->mux 
        || !this->sink) {
        std::cout << "Error: Failed to create HLS elements." << std::endl;
        exit(1);
    }

    // config interval -1 means no config frame and IS NECESSARY
    // cannot be set to 0
    g_object_set(G_OBJECT(this->parser),
                "config-interval", 1,
                NULL);

    std::string livestream_location = this->context->recordingDir + "/" + "livestream.m3u8";
    std::string segment_location = this->context->recordingDir + "/" + "segment%05d.ts";

    this->webroot = "/recordings/";
    #ifdef DEBUG
    this->webroot = this->webroot + ":8888";
    #endif
    g_object_set(G_OBJECT(this->sink),
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
    gst_bin_add_many(GST_BIN(pipeline),
                    this->queue,
                    this->parser,
                    this->mux,    
                    this->sink,
                    NULL);

    // Link the elements
    if (!gst_element_link_many(this->queue, 
                                this->parser,
                                this->mux, 
                                this->sink, NULL)) {
        std::cout << "Error: Could not link HLS elements." << std::endl;
        exit(1);
    }

    // Link tee to hls_queue
    GstPad *tee_hls_pad = gst_element_request_pad_simple(context->getSourceTee(), "src_%u");
    GstPad *queue_hls_pad = gst_element_get_static_pad(this->queue, "sink");

    if (gst_pad_link(tee_hls_pad, queue_hls_pad) != GST_PAD_LINK_OK) {
        std::cout << "Error: Could not link tee and HLS queue pads." << std::endl;
        exit(1);
    }

    gst_object_unref(queue_hls_pad);
    gst_object_unref(tee_hls_pad);

    std::cout << "HLS elements setup successfully. Web root " << webroot << std::endl;
}
