#include "LibcameraHlsPipeline.h"

LibcameraHlsPipeline::LibcameraHlsPipeline(const char* dir, int vid_duration, int* argc, char*** argv)
    : RecordingPipeline(dir, vid_duration, argc, argv) {
    setupRecordingPipeline();
}

void LibcameraHlsPipeline::setupRecordingPipeline() {
    setupLibcameraSrcAndTee();
    setupFileSinkElements(true);
    setupHlsElements();

    cout << "Gstreamer elements setup successfully." << endl;
}
