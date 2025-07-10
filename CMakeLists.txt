cmake_minimum_required(VERSION 3.16)
project( dashcam VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# pkg-config --libs opencv4
#find_package( OpenCV REQUIRED )
#include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories(src/)
# Find GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
include_directories(${GST_INCLUDE_DIRS})


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable( dashcam src/main.cc src/utilities.cc src/RecordingPipeline.cc src/TsFilePipelineSink.cc src/HlsPipelineSink.cc src/V4l2PipelineSource.cc src/LibcameraPipelineSource.cc src/CamService.cc )

# Set default value if not defined externally
set(RPI_MODE "" CACHE STRING "Build mode: RPI, ZERO, or empty for desktop")

# Conditional Raspberry Pi definitions
if(RPI_MODE)
    message(STATUS "Building for Raspberry Pi")
    target_compile_definitions(dashcam PRIVATE RPI_MODE)
elseif(RPI_ZERO_MODE)
    message(STATUS "Building for Raspberry Pi Zero")
    target_compile_definitions(dashcam PRIVATE RPI_ZERO_MODE)
elseif(EXPERIMENTAL_MODE)
    message(STATUS "Building on EXPERIMENTAL MODE. Setting EXPERIMENTAL_MODE compile definition.")
    target_compile_definitions(dashcam PRIVATE EXPERIMENTAL_MODE)
else()
    message(STATUS "Building for desktop")
endif()


if(NOT CMAKE_BUILD_TYPE OR NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -Wpedantic")
    # target_compile_definitions(dashcam PRIVATE DEBUG )
endif()

target_link_libraries( dashcam ${OpenCV_LIBS} ${GST_LIBRARIES})

install(TARGETS dashcam DESTINATION /usr/local/bin)
#install(FILES dashcam.conf DESTINATION /etc/dashcam)
