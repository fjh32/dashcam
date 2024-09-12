cmake_minimum_required(VERSION 3.5)
project( dashcam VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# pkg-config --libs opencv4
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(src/)
# Find GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})

# Find the Drogon package (this should work if Drogon is installed)
find_package(Drogon REQUIRED)


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable( dashcam src/main.cc src/utilities.cc src/RecordingPipeline.cc src/HttpServer.cc src/CamService.cc)

if(RPI_MODE STREQUAL "ON")
    target_compile_definitions(dashcam PRIVATE RPI_MODE)
endif()

if(NOT CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -Wpedantic")
    # target_compile_definitions(dashcam PRIVATE DEBUG )
endif()

target_link_libraries( dashcam ${OpenCV_LIBS} ${GST_LIBRARIES} Drogon::Drogon)
