// // #include "CamRecorder.h"
#include <iostream>
#include <signal.h>
#include <memory>

#include "GstRecordingPipeline.h"

using namespace std;

unique_ptr<GstRecordingPipeline> gstPipeline;

void catch_sigint(int signum);

int main(int argc, char* argv[]) {
    gstPipeline = make_unique<GstRecordingPipeline>("./recordings/", &argc, &argv);

    signal(SIGINT, catch_sigint);
    
    gstPipeline->startPipeline();
}

void catch_sigint(int signum)
{
    cout << "Exiting cleanly... " << signum << endl;
    gstPipeline->stopPipeline();

    exit(signum);
}
