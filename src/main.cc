#include <iostream>
#include <signal.h>
#include <memory>

#include "GstRecordingPipeline.h"
#include "CamService.h"

using namespace std;

unique_ptr<CamService> camservice;
unique_ptr<GstRecordingPipeline> gstRecordingPipeline;

void catch_signal(int signum);

int main(int argc, char* argv[]) {
    camservice = make_unique<CamService>( &argc, &argv);
    signal(SIGINT, catch_signal);
    signal(SIGTERM, catch_signal); // Termination request
    signal(SIGQUIT, catch_signal); // Ctrl+\ (Quit)
    signal(SIGHUP, catch_signal);  // Terminal closed
    
    camservice->mainLoop();
}

void catch_signal(int signum)
{
    cout << "Exiting cleanly. Received signal " << strsignal(signum) << endl;
    camservice->killMainLoop();
    exit(signum);
}
