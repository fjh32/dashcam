// // #include "CamRecorder.h"
#include <iostream>
#include <signal.h>
#include <memory>

// #include "CamService.h"
#include "GstRecordingPipeline.h"
#include "CamService.h"

using namespace std;

unique_ptr<CamService> camservice;
unique_ptr<GstRecordingPipeline> gstRecordingPipeline;

void catch_sigint(int signum);

int main(int argc, char* argv[]) {
    camservice = make_unique<CamService>( &argc, &argv);
    // gstRecordingPipeline = make_unique<GstRecordingPipeline>("./recordings/", &argc, &argv);
    signal(SIGINT, catch_sigint);
    
    camservice->mainLoop();
    // camservice->startRecording();
    // gstRecordingPipeline->startPipeline();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cout << "killing main loop" << endl;
    camservice->killMainLoop();
    // gstRecordingPipeline->stopPipeline();
}

void catch_sigint(int signum)
{
    cout << "Exiting cleanly... " << signum << endl;
    camservice->killMainLoop();
    // camservice->stopPipeline();
    // camservice->stopRecording();
    // gstRecordingPipeline->stopPipeline();
    exit(signum);
}
