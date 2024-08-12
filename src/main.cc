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
    cout << "Exiting cleanly...\n";
    

    if(gstPipeline->pipelineRunning) {
        debugPrint("Stopping pipeline");
        debugPrint("Sending EOS event");
        gst_element_send_event(gstPipeline->gstData->pipeline, gst_event_new_eos());
        
        debugPrint("Getting pipeline state");

        GstState state;
        GstState pending;

        // Get the pipeline state, blocking until the state change is complete
        GstStateChangeReturn ret = gst_element_get_state(gstPipeline->gstData->pipeline, &state, &pending, GST_CLOCK_TIME_NONE);

        if (ret == GST_STATE_CHANGE_SUCCESS) {
        std::cout << "Pipeline is now in state: " << gst_element_state_get_name(state) << std::endl;
        } else if (ret == GST_STATE_CHANGE_ASYNC) {
        std::cout << "Pipeline is still changing state asynchronously to: " << gst_element_state_get_name(pending) << std::endl;
        } else {
        std::cerr << "Failed to change pipeline state." << std::endl;
        }

        debugPrint("Setting pipeline state to NULL");
        gst_element_set_state(gstPipeline->gstData->pipeline, GST_STATE_NULL);
        gstPipeline->pipelineRunning = false;
    }


    exit(signum);
}
