#include "HlsPipeline.h"



void HlsPipeline::setupHlsElements() {
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