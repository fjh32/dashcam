#include "CamRecorder.h"
using namespace cv;

CamRecorder::CamRecorder() {
    isRecording = false;
    this->recordingDir = RECORDING_DIR;
    this->recordingSaveDir = RECORDING_SAVE_DIR;

    this->cap = VideoCapture(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the camera." << std::endl;
        exit(1);
    }
    this->width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    this->height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    std::cout << "Frame width: " << width << std::endl << "Frame height: " << height << std::endl;

    makeRecordingDirs();

    writer = setupNewVideoWriter();
    if(!writer.isOpened()) {
        std::cerr << "Error: Could not open the output video file." << std::endl;
        exit(1);
    }

    createListeningPipe();
}

CamRecorder::~CamRecorder() {
    // Add destructor code here
    removeListeningPipe();
    cap.release();
    writer.release();
}

void CamRecorder::recordingLoop() {
    std::cout << "Recording loop started\n";
    isRecording = true;
    recordingThread = std::thread(&CamRecorder::startRecording, this);
    cleanupThread = std::thread(&CamRecorder::cleanupThreadLoop, this);
    listenOnPipe();
}

void CamRecorder::startRecording() {
    
    #ifdef DEBUG
    std::cout << "startRecording()\n";
    #endif
    
    cv::Mat frame;
    auto start_time = now_steady();
    while (isRecording) {
        cap >> frame;
        auto last_frame_capture_time = std::chrono::steady_clock::now();
        if (frame.empty()) {
            std::cerr << "Error: Could not capture a frame." << std::endl;
            break;
        }
        
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            writer.write(frame);
            // cycle video file every VIDEO_DURATION seconds
            auto current_time = now_steady();
            auto duration = duration_ms(start_time, current_time);
            if (duration / 1000 >= VIDEO_DURATION) { // ms to s
                std::cout << "Making new video file. Previous Duration in ms: " << duration << std::endl;
                writer.release();
                writer = setupNewVideoWriter();
                start_time = current_time;
            }
            // release lock
        }
        auto end = std::chrono::steady_clock::now();
        auto duration = duration_ms(last_frame_capture_time, end);
        std::time_t time_to_sleep = std::max(0l, 1000/FRAME_RATE - duration);
        std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep)); 
    }

    #ifdef DEBUG
    std::cout << "EXITING startRecording(). this->isRecording " << (isRecording ? "true" : "false") << std::endl;
    #endif
}

void CamRecorder::stopRecording() {
    isRecording = false;
    recordingThread.join();
    cleanupThread.join();
    std::cout << "Recording stopped\n";
}

void CamRecorder::cleanupThreadLoop() {

    #ifdef DEBUG
    std::cout << "cleanupThreadLoop()\n";
    #endif

    time_t start_time = now();
    std::time_t threshold_time = start_time - DELETE_OLDER_THAN;
    bool first = true;
    while(isRecording) {

        time_t current_time = now();
        if (current_time - start_time >= DELETE_OLDER_THAN || first) {
            first = false;
            start_time = current_time;
            deleteOlderFiles(threshold_time);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));   
    }
    #ifdef DEBUG
    std::cout << "Closing cleanupThreadLoop()\n";
    #endif
}

void CamRecorder::deleteOlderFiles(std::time_t threshold_time) {
    auto dir_contents = getRecordingDirContents();
    for(auto &entry : dir_contents) {
        std::time_t file_timestamp = time_t_from_direntry(entry);

        if (file_timestamp < threshold_time) {
            std::filesystem::remove(entry);
            std::cout << "Deleted file: " << entry.path().filename().string() << std::endl;
        }
    }
}

void CamRecorder::createListeningPipe() {
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

void CamRecorder::removeListeningPipe() {
    // Add removeListeningPipe code here
    if (unlink(PIPE_NAME) == -1) {
        std::cerr << "Error removing named pipe: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::cout << "Named pipe removed" << std::endl;
    
}

void CamRecorder::listenOnPipe() {
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
        if (receivedMessage == "stop") {
            stopRecording();
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

void CamRecorder::saveRecordings(int seconds_back_to_save) {
    std::time_t current_time = now();
    std::time_t threshold_time = current_time - seconds_back_to_save;

    // do this so we don't hit infinite loop of finding currentlyRecordingVideoName

    // if seconds_back_to_save is less than VIDEO_DURATION, then we should save only the current recording
    // otherwise, we should save everything within threshold_time
    if(seconds_back_to_save < VIDEO_DURATION) {
        std::string filename = currentlyRecordingVideoName;
        std::string baseFilename = filename.substr(filename.find_last_of("/\\") + 1);
        std::string savePath = recordingSaveDir + baseFilename;

        std::lock_guard<std::mutex> lock(writeMutex);
        writer.release();
        writer = setupNewVideoWriter();

        std::filesystem::copy_file(filename, savePath, std::filesystem::copy_options::overwrite_existing);
        std::cout << "Saved file: " << filename << std::endl;
        
        return;
    } else {

        auto dir_contents = getRecordingDirContents();
        for (const auto& entry : dir_contents) {
                std::time_t file_timestamp = time_t_from_direntry(entry);

                if (file_timestamp > threshold_time) {
                    std::string filename = entry.path().filename().string();

                    if (filename.find(currentlyRecordingVideoName) != std::string::npos) {
                        std::lock_guard<std::mutex> lock(writeMutex);
                        writer.release();
                        writer = setupNewVideoWriter();
                    }

                    std::string savePath = recordingSaveDir + filename;
                    std::filesystem::copy_file(entry.path(), savePath, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Saved file: " << filename << std::endl;
                }
        }
    }
}


cv::VideoWriter CamRecorder::setupNewVideoWriter() {
    std::string newVideoName = recordingDir + "output_" + formatted_time() + ".mkv";
    auto newWriter = VideoWriter(newVideoName, VideoWriter::fourcc('X', '2', '6', '4'), FRAME_RATE, Size(this->width, this->height));
    currentlyRecordingVideoName = newVideoName;
    return newWriter;
}

void CamRecorder::makeRecordingDirs() {
    // Add makeRecordingDirs code here
    

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

std::vector<std::filesystem::directory_entry> CamRecorder::getRecordingDirContents() {
    std::vector<std::filesystem::directory_entry> dir_contents;
    for (const auto& entry : std::filesystem::directory_iterator(recordingDir)) {
        if (std::filesystem::is_regular_file(entry)) {
            dir_contents.push_back(entry);
        }
    }
    return dir_contents;
}