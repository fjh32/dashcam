#include "V4l2HlsPipeline.h"

V4l2HlsPipeline::V4l2HlsPipeline(const char* dir, int vid_duration, int* argc, char*** argv)
    : RecordingPipeline(dir, vid_duration, argc, argv) {
    wait_for_video_device();
    setupRecordingPipeline();
}

void V4l2HlsPipeline::setupRecordingPipeline() {
    setupV4l2SrcAndTee();
    setupFileSinkElements(true);
    setupHlsElements();

    cout << "Gstreamer elements setup successfully." << endl;
}
