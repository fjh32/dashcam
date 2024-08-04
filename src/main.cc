#include "CamRecorder.h"
#include <iostream>
#include <signal.h>

using namespace std;

unique_ptr<CamRecorder> camRecorder;

void catch_sigint(int signum);

int main() {
    camRecorder = make_unique<CamRecorder>();
    signal(SIGINT, catch_sigint);
    
    camRecorder->recordingLoop();
}

void catch_sigint(int signum)
{
    cout << "Exiting cleanly...\n";
    camRecorder->stopRecording();
    exit(0);
}