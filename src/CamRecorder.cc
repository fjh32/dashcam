#include "CamRecorder.h"
using namespace cv;

CamRecorder::CamRecorder() {
    isRecording = false;

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

cv::VideoWriter CamRecorder::setupNewVideoWriter() {
    std::string newVideoName = recordingDir + "output_" + formatted_time() + ".mkv";
    auto newWriter = VideoWriter(newVideoName, VideoWriter::fourcc('X', '2', '6', '4'), FRAME_RATE, Size(width, height));
    currentlyRecordingVideoName = newVideoName;
    return newWriter;
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

void CamRecorder::makeRecordingDirs() {
    // Add makeRecordingDirs code here
    this->recordingDir = RECORDING_DIR;
    this->recordingSaveDir = RECORDING_SAVE_DIR;

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

void CamRecorder::removeListeningPipe() {
    // Add removeListeningPipe code here
    if (unlink(PIPE_NAME) == -1) {
        std::cerr << "Error removing named pipe: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::cout << "Named pipe removed" << std::endl;
    
}

void CamRecorder::listenOnPipe() {
    std::string receivedMessage;
    if (!pipe) {
        std::cerr << "Error opening named pipe: " << strerror(errno) << std::endl;
        exit(1);
    }
    std::regex saveRegex("save:(\\d+)");
    while(1) {
        std::ifstream pipe(PIPE_NAME);
        std::getline(pipe, receivedMessage);
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

void CamRecorder::recordingLoop() {
    std::cout << "Recording loop started\n";
    isRecording = true;
    recordingThread = std::thread(&CamRecorder::startRecording, this);
    cleanupThread = std::thread(&CamRecorder::cleanupThreadLoop, this);
    listenOnPipe();
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
            auto dir_contents = getRecordingDirContents();
            for(auto &entry : dir_contents) {
                auto file_time = std::filesystem::last_write_time(entry);
                // Convert to system clock time point
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    file_time - std::filesystem::file_time_type::clock::now()
                    + std::chrono::system_clock::now());
                // Convert to time_t
                std::time_t file_timestamp = std::chrono::system_clock::to_time_t(sctp);

                if (file_timestamp < threshold_time) {
                    std::filesystem::remove(entry);
                    std::cout << "Deleted file: " << entry.path().filename().string() << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));   
    }
    #ifdef DEBUG
    std::cout << "Closing cleanupThreadLoop()\n";
    #endif
}

void CamRecorder::startRecording() {
    #ifdef DEBUG
    std::cout << "startRecording()\n";
    #endif

    
    cv::Mat frame;
    std::time_t start_time = now();
    while (isRecording) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not capture a frame." << std::endl;
            break;
        }
        
        {
            std::lock_guard<std::mutex> lock(writeMutex);
            writer.write(frame);

            std::time_t current_time = now();
            if (current_time - start_time >= VIDEO_DURATION) {
                std::cout << current_time << " Current time. Start time: " << start_time << std::endl;
                writer.release();
                writer = setupNewVideoWriter();
                start_time = current_time;
            }
        }
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

std::vector<std::filesystem::directory_entry> CamRecorder::getRecordingDirContents() {
    std::vector<std::filesystem::directory_entry> dir_contents;
    for (const auto& entry : std::filesystem::directory_iterator(recordingDir)) {
        if (std::filesystem::is_regular_file(entry)) {
            dir_contents.push_back(entry);
        }
    }
    return std::move(dir_contents);
}

void CamRecorder::saveRecordings(int seconds_back_to_save) {
    // Add save recording code here

    std::string saveDir = recordingSaveDir;

    std::time_t current_time = now();
    std::time_t threshold_time = current_time - seconds_back_to_save;

    // do this so we don't hit infinite loop of finding currentlyRecordingVideoName
    auto dir_contents = getRecordingDirContents();

    for (const auto& entry : dir_contents) {
            auto file_time = std::filesystem::last_write_time(entry);
            // Convert to system clock time point
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                file_time - std::filesystem::file_time_type::clock::now()
                + std::chrono::system_clock::now());
             // Convert to time_t
            std::time_t file_timestamp = std::chrono::system_clock::to_time_t(sctp);


            if (file_timestamp > threshold_time) {
                std::string filename = entry.path().filename().string();

                if (filename.find(currentlyRecordingVideoName) != std::string::npos) {
                    std::lock_guard<std::mutex> lock(writeMutex);
                    writer.release();
                    writer = setupNewVideoWriter();
                }

                std::string savePath = saveDir + filename;
                std::filesystem::copy_file(entry.path(), savePath, std::filesystem::copy_options::overwrite_existing);
                std::cout << "Saved file: " << filename << std::endl;
            }
        
    }

    
}