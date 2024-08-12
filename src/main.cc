// // #include "CamRecorder.h"
#include <iostream>
#include <signal.h>

#include "CameraRecorder.h"

using namespace std;

// unique_ptr<CamRecorder> camRecorder;

unique_ptr<CameraRecorder> cameraRecorder;
void catch_sigint(int signum);

int main(int argc, char* argv[]) {
    cameraRecorder = make_unique<CameraRecorder>(argc, argv);

    // camRecorder = make_unique<CamRecorder>();
    signal(SIGINT, catch_sigint);
    
    // camRecorder->recordingLoop();
    // std::atexit([]() {
    //     cameraRecorder->~CameraRecorder();
    // });
    cameraRecorder->mainLoop();
    // std::thread thr(&CameraRecorder::mainLoop, cameraRecorder.get());
    // sleep(5);
    // cameraRecorder->~CameraRecorder();
}

void catch_sigint(int signum)
{
    // cameraRecorder->~CameraRecorder();
    // cameraRecorder->kill();
    // cameraRecorder->stopPipeline();
    cout << "Exiting cleanly...\n";

    exit(signum);
}
