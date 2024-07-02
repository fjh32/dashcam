#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#define FRAME_RATE 10

class CamRecorder {
    public:
        CamRecorder();  // Constructor
        ~CamRecorder(); // Destructor

        void recordingLoop();

    private:
        bool isRecording;
        cv::VideoCapture cap;
        cv::VideoWriter writer;
        double height, width;
        // Add private member variables or helper functions here
        void startRecording();
        void stopRecording();
        void saveRecording(const std::string& filename);
};