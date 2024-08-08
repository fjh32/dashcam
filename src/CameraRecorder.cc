#include "CameraRecorder.h"

///////////////////////////////////////////////////////////////////////
using namespace cv;

GstData::GstData() {}

GstData::~GstData() {
    #ifdef DEBUG
    std::cout << "Destructor called for GstData" << std::endl;
    #endif
}

///////////////////////////////////////////////////////////////////////

CameraRecorder::CameraRecorder(int argc, char* argv[]) {
    isRecording = false;
    killed = false;
    recordingDir = RECORDING_DIR;
    recordingSaveDir = RECORDING_SAVE_DIR;
    setupGstElements(argc, argv);
    makeRecordingDirs();
    createListeningPipe();
    std::cout << "Gstreamer initialized" << std::endl;
    
}

CameraRecorder::~CameraRecorder() {
    // Add destructor code here
    #ifdef DEBUG
    std::cout << "Destructor called for Camera Recorder" << std::endl;
    #endif

    if(!killed) {
        kill();
    }
    gst_object_unref(gstData->pipeline);
}

void CameraRecorder::mainLoop() {
    recordingThread = std::thread(&CameraRecorder::startRecording, this);
    cleanupThread = std::thread(&CameraRecorder::cleanupThreadLoop, this);
    listenOnPipe();
}

void CameraRecorder::startRecording() {
    std::cout << "Starting Recording Loop\n";

    ////  Test out start/stop
    // pipelineThread = std::thread(&CameraRecorder::startPipeline, this);
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    // stopPipeline();
    ///////////////////////
    
    isRecording = true;
    pipelineThread = std::thread(&CameraRecorder::startPipeline, this);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    while(isRecording) {
        auto current_time = now_steady();
        auto dur = duration<std::chrono::seconds>(currentVideoStartTime, current_time);
        if(dur >= VIDEO_DURATION) {
            std::lock_guard<std::mutex> lock(recordingSwapMutex);
            stopPipeline();
            pipelineThread = std::thread(&CameraRecorder::startPipeline, this);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    
    std::cout << "Exiting recordingLoop()" << std::endl;
}

void CameraRecorder::startPipeline() {
    #ifdef DEBUG
    std::cout << "startPipeline()\n";
    #endif

    currentlyRecordingVideoName = makeNewFilename();
    g_object_set(gstData->sink, "location", currentlyRecordingVideoName.c_str(), NULL);
    std::cout << "Recording to file: " << currentlyRecordingVideoName << std::endl;
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
    std::cout << "EXITING startPipeline()." << std::endl;
    #endif
}

void CameraRecorder::stopPipeline() {
    // Add code here
    std::cout << "stopPipeline(): Stopping recording" << std::endl;
    
    gst_element_send_event(gstData->pipeline, gst_event_new_eos());
    pipelineThread.join();
    
    std::cout << "stopPipeline(): Recording stopped" << std::endl;
}

void CameraRecorder::kill() {
    std::cout<< "starting kill(): Killing entire thing" << std::endl;
    stopPipeline();
    isRecording = false;
    recordingThread.join();
    cleanupThread.join();
    removeListeningPipe();
    killed = true;

    std::cout << "ending kill(): Recording loop killed" << std::endl;
}


void CameraRecorder::saveRecordings(int seconds_back_to_save) {
    auto current_time = now();
    auto threshold_time = current_time - seconds_back_to_save;
    if(seconds_back_to_save <= VIDEO_DURATION) {
        std::string filename = currentlyRecordingVideoName;
        std::string baseFilename = filename.substr(filename.find_last_of("/\\") + 1);
        std::string savePath = recordingSaveDir + baseFilename;
        std::lock_guard<std::mutex> lock(recordingSwapMutex);
        stopPipeline();
        pipelineThread = std::thread(&CameraRecorder::startPipeline, this);
        std::filesystem::copy_file(filename, savePath, std::filesystem::copy_options::overwrite_existing);
        std::cout << "Saved file: " << savePath << std::endl;

    } else {
        auto dir_contents = getRecordingDirContents();
        for(const auto& entry :dir_contents) {
            std::time_t file_timestamp = time_t_from_direntry(entry);
            if (file_timestamp > threshold_time) {
                    std::string filename = entry.path().filename().string();

                    if (currentlyRecordingVideoName.find(filename) != std::string::npos) {
                        std::lock_guard<std::mutex> lock(recordingSwapMutex);
                        stopPipeline();
                        pipelineThread = std::thread(&CameraRecorder::startPipeline, this);
                    }

                    std::string savePath = recordingSaveDir + filename;
                    std::filesystem::copy_file(entry.path(), savePath, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Saved file: " << filename << std::endl;
                }
        }
    }
}



void CameraRecorder::setupGstElements(int argc, char* argv[]) {
    // Add code here
    gst_init(&argc, &argv);

    gstData = std::make_unique<GstData>();

    gstData->pipeline = gst_pipeline_new("recording_pipeline");
    #ifndef RPI_MODE
    gstData->source = gst_element_factory_make("v4l2src", "source");
    #else
    std::cout << "rpi libcamera src mode\n";
    gstData->source = gst_element_factory_make("rpicamsrc", "source");
    #endif
    //g_object_set(gstData->source, "device", "/dev/video0", NULL);
    gstData->queue = gst_element_factory_make("queue", "queue_thread");
    gstData->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    gstData->videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    #ifdef RPI_MODE
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    std::cout << "Using omxh264enc for RPI_MODE" << std::endl;
    #else
    gstData->encoder = gst_element_factory_make("x264enc", "encoder");
    std::cout << "Using x264enc. NOT RPI_MODE" << std::endl;
    #endif
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

    gst_bin_add_many(GST_BIN(gstData->pipeline), gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL);
    if(!gst_element_link_many(gstData->source, gstData->queue, gstData->capsfilter, gstData->videoconvert, gstData->encoder, gstData->muxer, gstData->sink, NULL)) {
        std::cerr << "Error: Could not link gstreamer elements." << std::endl;
        exit(1);
    }
}

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
            eosSwitch = true;
            break;
        default:
            break;
    }
    gst_message_unref(msg);
    return eosSwitch;
}

void CameraRecorder::createListeningPipe() {
    // Add createListeningPipe code here
    if (mkfifo(PIPE_NAME, 0666) == -1) {
        std::string errstring = strerror(errno);
        if (errstring.find("File exists") != std::string::npos) {
            std::filesystem::remove(PIPE_NAME);
            createListeningPipe();
        } else {
            std::cerr << "Error creating named pipe: " << strerror(errno) << std::endl;
            exit(1);
        }
    }
    std::cout << "Named pipe created at " << PIPE_NAME << std::endl;
}

void CameraRecorder::removeListeningPipe() {
    // Add removeListeningPipe code here
    if (unlink(PIPE_NAME) == -1) {
        std::cerr << "Error removing named pipe: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::cout << "Named pipe removed" << std::endl;
}

void CameraRecorder::listenOnPipe() {
    #ifdef DEBUG
    std::cout << "listenOnPipe()\n";
    #endif

    std::string receivedMessage;
    std::regex saveRegex("save:(\\d+)");

    while(1) {
        std::ifstream pipe(PIPE_NAME);
        if (!pipe) {
            std::cerr << "Error opening named pipe: " << strerror(errno) << std::endl;
            exit(1);
        }

        std::getline(pipe, receivedMessage); // blocking read on pipe
        std::cout << "Received message on pipe: " << receivedMessage << std::endl;

        std::smatch match;
        if (receivedMessage == "kill") {
            kill();
            break;
        }
        else if (std::regex_match(receivedMessage, match, saveRegex)) {
            int value = std::stoi(match[1].str());
            std::cout << "Received save command with value: " << value << std::endl;
            saveRecordings(value);
        }
        pipe.close(); // close the pipe after reading
    }

    #ifdef DEBUG
    std::cout << "Exiting listenOnPipe()\n";
    #endif
}

void CameraRecorder::cleanupThreadLoop() {
    #ifdef DEBUG
    std::cout << "cleanupThreadLoop()\n";
    #endif

    time_t start_time = now();
    std::time_t threshold_time = start_time - DELETE_OLDER_THAN;
    while(isRecording) {

        time_t current_time = now();
        auto diff = current_time - start_time;
        if (diff > VIDEO_DURATION) {
            deleteOlderFiles(threshold_time);
            start_time = current_time;
            threshold_time = start_time - DELETE_OLDER_THAN;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));   
    }
    #ifdef DEBUG
    std::cout << "Closing cleanupThreadLoop()\n";
    #endif
}

void CameraRecorder::deleteOlderFiles(std::time_t threshold_time) {
    auto dir_contents = getRecordingDirContents();
    for(auto &entry : dir_contents) {
        std::time_t file_timestamp = time_t_from_direntry(entry);

        if (file_timestamp < threshold_time) {
            std::filesystem::remove(entry);
            std::cout << "Cleaned up file: " << entry.path().filename().string() << std::endl;
        }
    }
}

std::string CameraRecorder::makeNewFilename() {
    return recordingDir + "output_" + formatted_time() + ".mkv";

}

void CameraRecorder::makeRecordingDirs() {
    if (!std::filesystem::exists(recordingDir)) {
        if (!std::filesystem::create_directory(recordingDir)) {
            std::cerr << "Error: Could not create recording directory." << std::endl;
            exit(1);
        }
    }
    if (!std::filesystem::exists(recordingSaveDir)) {
        if (!std::filesystem::create_directory(recordingSaveDir)) {
            std::cerr << "Error: Could not create recording save directory." << std::endl;
            exit(1);
        }
    }
}

std::vector<std::filesystem::directory_entry> CameraRecorder::getRecordingDirContents() {
    std::vector<std::filesystem::directory_entry> dir_contents;
    for (const auto& entry : std::filesystem::directory_iterator(recordingDir)) {
        if (std::filesystem::is_regular_file(entry)) {
            dir_contents.push_back(entry);
        }
    }
    return dir_contents;
}
///////////////////////////////////////////////////////////////////////
