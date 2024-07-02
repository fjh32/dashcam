#include "CamRecorder.h"
#include <thread>
using namespace cv;

CamRecorder::CamRecorder() {
    isRecording = false;
    // Add constructor code here
    this->cap = VideoCapture(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the camera." << std::endl;
        exit(1);
    }
    this->width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    this->height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    // this->width = 1920;
    // this->height = 1080;

    std::cout << "Frame width: " << width << std::endl << "Frame height: " << height << std::endl;

    writer = VideoWriter("output.mkv", VideoWriter::fourcc('X', '2', '6', '4'), FRAME_RATE, Size(width, height));

    if(!writer.isOpened()) {
        std::cerr << "Error: Could not open the output video file." << std::endl;
        exit(1);
    }
}

CamRecorder::~CamRecorder() {
    // Add destructor code here
    cap.release();
    writer.release();
}

void CamRecorder::recordingLoop() {
    std::cout << "Recording loop started\n";
    std::thread recordingThread( &CamRecorder::startRecording, this);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stopRecording();
    recordingThread.join();
}

void CamRecorder::startRecording() {
    isRecording = true;
    cv::Mat frame;
    while (isRecording) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Could not capture a frame." << std::endl;
            break;
        }
        writer.write(frame);
    }
}

void CamRecorder::stopRecording() {
    isRecording = false;
}

void CamRecorder::saveRecording(const std::string& filename) {
    // Add save recording code here
}