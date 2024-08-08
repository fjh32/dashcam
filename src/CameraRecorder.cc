#include "CameraRecorder.h"

///////////////////////////////////////////////////////////////////////
using namespace cv;

GstData::GstData() {}

GstData::~GstData() {
    std::cout << "Destructor called for GstData" << std::endl;
}

///////////////////////////////////////////////////////////////////////

/*
mainloop()
    start thread to listenOnPipe()
    start thread to cleanup old files - same logic as we had before
    thread for recordingLoop


 */
void CameraRecorder::mainLoop() {
    // std::thread recordingThread(&CameraRecorder::recordingLoop, this);
    // recordingThread.join();
    // recordingLoopThread = std::thread(&CameraRecorder::recordingLoop, this);

    // recordingLoopThread.join();
    recordingLoop();
}

/*
recordingLoop()
    thr = thread for startrecording()
    // starttime
    while() {
        mutex lock acquire videostarttime
        if currenttime - videostarttime >= VIDEO_DURATION
            stoprecording()
            thr = thread for startrecording()
                  
        sleep(1s)
    }

*/
void CameraRecorder::recordingLoop() {
    std::cout << "Starting Recording Loop\n";
    //  Test out start/stop
    // recordingThread = std::thread(&CameraRecorder::startRecording, this);
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    // stopRecording();
    ///////////////////////
    
    isRecording = true;
    recordingThread = std::thread(&CameraRecorder::startRecording, this);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while(isRecording) {
        auto current_time = now_steady();
        auto dur = duration<std::chrono::seconds>(currentVideoStartTime, current_time);
        if(dur >= VIDEO_DURATION) {
            std::lock_guard<std::mutex> lock(recordingSwapMutex);
            stopRecording();
            recordingThread = std::thread(&CameraRecorder::startRecording, this);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    stopRecording();
    std::cout << "Exiting recordingLoop()" << std::endl;

}

/*
startrecording()
    currentlyRecordingVideoName = create new video file
    start video - state = PLAYING
    mutex.lock acquire and set videostarttime

    while (isRecording)
        get bus messages
        if (message is EOS)
            break;
    stop video - state = NULL
    */
void CameraRecorder::startRecording() {
    #ifdef DEBUG
    std::cout << "startRecording()\n";
    #endif

    currentlyRecordingVideoName = makeNewFilename();
    g_object_set(gstData->sink, "location", currentlyRecordingVideoName.c_str(), NULL);
    gst_element_set_state(gstData->pipeline, GST_STATE_PLAYING);

    currentVideoStartTime = now_steady();
    gstData->bus = gst_element_get_bus(gstData->pipeline);
    while (true) {
        GstMessage *msg = gst_bus_timed_pop_filtered(gstData->bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (msg != nullptr) {
            if(handleBusMessage(msg)) {
                
                break;
            }
        }
        gst_message_unref(msg);
    }
    
    gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
    gst_object_unref(gstData->bus);
    #ifdef DEBUG
    std::cout << "EXITING startRecording()." << std::endl;
    #endif
}

/*
stoprecording()
    send eos to pipeline
    thr.join() // wait for recording to stop
*/
void CameraRecorder::stopRecording() {
    // Add code here
    std::cout << "stopRecording(): Stopping recording" << std::endl;
    
    gst_element_send_event(gstData->pipeline, gst_event_new_eos());
    recordingThread.join();
    
    std::cout << "stopRecording(): Recording stopped" << std::endl;
}

void CameraRecorder::kill() {
    std::cout<< "Killing entire thing" << std::endl;
    // stopRecording();
    isRecording = false;
    // gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
}

/*
Ideal flow:

saveRecordings(saveseconds) // on pipe receive save:300
    currenttime
    if savetime < videoduration
        filetosave = currentlyRecordingVideoName
        stoprecording()
        thr = thread for startrecording()
        save filetosave to recordingSaveDir
    else
        get list of files in recordingDir where time > threshold time
        save each file f to recordingSaveDir
            if filetosave == currentlyRecordingVideoName
                stoprecording()
                thr = thread for startrecording()
            f.save file to recordingSaveDir
*/

CameraRecorder::CameraRecorder(int argc, char* argv[]) {
    // isRecording = true;
    recordingDir = RECORDING_DIR;
    recordingSaveDir = RECORDING_SAVE_DIR;

    setupGstElements(argc, argv);
    std::cout << "Gstreamer initialized" << std::endl;
    
}

CameraRecorder::~CameraRecorder() {
    // Add destructor code here
    std::cout << "Destructor called for Camera Recorder" << std::endl;
    gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
    gst_object_unref(gstData->pipeline);
}

void CameraRecorder::setupGstElements(int argc, char* argv[]) {
    // Add code here
    gst_init(&argc, &argv);

    gstData = std::make_unique<GstData>();

    gstData->pipeline = gst_pipeline_new("recording_pipeline");

    gstData->source = gst_element_factory_make("v4l2src", "source");
    gstData->queue = gst_element_factory_make("queue", "queue_thread");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    gstData->muxer = gst_element_factory_make("matroskamux", "mux");
    gstData->sink = gst_element_factory_make("filesink", "sink");

    if ( !gstData->pipeline || !gstData->source || !gstData->queue || !gstData->capsfilter || !gstData->videoconvert ||!gstData->encoder  || !gstData->muxer || !gstData->sink) {
        std::cerr << "Error: Could not create gstreamer elements." << std::endl;
        exit(1);
    }

    // $ v4l2-ctl --list-formats-ext // checks available formats for webcam
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 640,
        "height", G_TYPE_INT, 480,
        "framerate", GST_TYPE_FRACTION, 30, 1,
        nullptr);
    g_object_set(gstData->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // // gstData->bus = gst_element_get_bus(gstData->pipeline);
    // currentlyRecordingVideoName = makeNewFilename();

    // g_object_set(gstData->sink, "location", currentlyRecordingVideoName.c_str(), NULL);

    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL);
    if(!gst_element_link_many(gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL)) {
        std::cerr << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}

// void CameraRecorder::startRecording() {
//     #ifdef DEBUG
//     std::cout << "startRecording()\n";
//     #endif
    
//     #ifdef DEBUG
//     std::cout << "EXITING startRecording(). this->isRecording " << (isRecording ? "true" : "false") << std::endl;
//     #endif
// }

bool CameraRecorder::handleBusMessage(GstMessage *msg) {
    // Add code here
    bool eosSwitch = false;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
            eosSwitch = true;
            break;
        }
        case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            // switchFileSink();
            // gst_element_set_state(gstData->pipeline, GST_STATE_NULL);
            eosSwitch = true;
            break;
        default:
            break;
    }
    gst_message_unref(msg);
    return eosSwitch;
}

void CameraRecorder::switchFileSink() {

}

// void CameraRecorder::stopRecording() {
//     // Add code here
//     std::cout << "Stopping recording" << std::endl;
//     isRecording = false;
// }

std::string CameraRecorder::makeNewFilename() {
    return recordingDir + "output_" + formatted_time() + ".mkv";

}
///////////////////////////////////////////////////////////////////////
