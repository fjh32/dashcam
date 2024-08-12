#include "CamService.h"

CamService::CamService(int* argc, char** argv[]) {
    recordingDir = RECORDING_DIR;
    recordingSaveDir = RECORDING_SAVE_DIR;
    makeDir(recordingDir.c_str());
    makeDir(recordingSaveDir.c_str());
    

    gstRecordingPipeline = make_unique<GstRecordingPipeline>(RECORDING_DIR, argc, argv);
    running = false;
}

CamService::~CamService() {
    // killMainLoop();
}

void CamService::mainLoop() {
    // running = true;
    // while(running) {
    // }
    safeToEndMainLoop = false;
    running = true;
    recordingThread = std::thread(&CamService::recordingLoop, this);
    cout << "Exiting main loop" << endl;
}

void CamService::killMainLoop() {
    running = false;
    recordingThread.join();
    cout << "Exiting kill function" << endl;
}

/// private functions ///////////////////////////////////////////

void CamService::recordingLoop() {
    
    currentVideoStartTime = now_steady();
    gstRecordingPipeline->startPipeline();

    while(running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        auto current_time = now_steady();
        auto dur = duration<std::chrono::seconds>(currentVideoStartTime, current_time);
        if(dur >= VIDEO_DURATION) {
            gstRecordingPipeline->stopPipeline();
            gstRecordingPipeline->startPipeline();
            currentVideoStartTime = current_time;
        }
    }
    gstRecordingPipeline->stopPipeline();
    safeToEndMainLoop = true;
}

bool CamService::isRecording() {
    return gstRecordingPipeline->pipelineRunning;
}