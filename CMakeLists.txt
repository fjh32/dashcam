cmake_minimum_required(VERSION 3.5)
project( dashcam )
set(CMAKE_CXX_STANDARD 20)
# pkg-config --libs opencv4
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(src/)
add_executable( dashcam src/main.cc src/CamRecorder.cc)
target_link_libraries( dashcam ${OpenCV_LIBS} )
