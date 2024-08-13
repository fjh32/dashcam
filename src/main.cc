#include <iostream>
#include <signal.h>
#include <memory>

#include "CamService.h"
#include "RecordingPipeline.h"
using namespace std;

unique_ptr<CamService> camservice;
// unique_ptr<GstRecordingPipeline> gstRecordingPipeline;
// unique_ptr<RecordingPipeline> recordingPipeline;
void catch_signal(int signum);

int main(int argc, char* argv[]) {
    camservice = make_unique<CamService>( &argc, &argv);
    // recordingPipeline = make_unique<RecordingPipeline>("./recordings", &argc, &argv);
    signal(SIGINT, catch_signal);
    signal(SIGTERM, catch_signal); // Termination request
    signal(SIGQUIT, catch_signal); // Ctrl+\ (Quit)
    signal(SIGHUP, catch_signal);  // Terminal closed
    
    camservice->mainLoop();
    // camservice->mainLoop();
    // cout << "Starting pipeline" << endl;
    // recordingPipeline->startPipeline();
    // while(true) {
    //     string input;
    //     cin >> input;
    //     if (input.find('n') != string::npos) {
    //         cout << "Creating new video" << endl;
    //         recordingPipeline->createNewVideo();
    //     } else if (input.find('q') != string::npos) {
    //         cout << "Stopping pipeline" << endl;
    //         recordingPipeline->stopPipeline();
    //         break;
    //     }
    // }
}

void catch_signal(int signum)
{
    cout << "Exiting cleanly. Received signal " << strsignal(signum) << endl;
    // recordingPipeline->stopPipeline();
    camservice->killMainLoop();
    exit(signum);
}