cmake_minimum_required(VERSION 3.5)
project(dashcam)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    utilities.cc
    RecordingPipeline.cc
    HlsPipeline.cc
    TsFileSinkPipeline.cc
    LibcameraHlsPipeline.cc
    V4l2HlsPipeline.cc
    CamService.cc
    main.cc
)

set(HEADERS
    utilities.h
    RecordingPipeline.h
    HlsPipeline.h
    TsFileSinkPipeline.h
    LibcameraHlsPipeline.cc
    V4l2HlsPipeline.h
    CamService.h
)

set(LIBRARIES
    # Add libraries here
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})