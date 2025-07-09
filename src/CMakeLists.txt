cmake_minimum_required(VERSION 3.5)
project(dashcam)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    utilities.cc
    RecordingPipeline.cc
    HlsPipeline.cc
    TsFileSinkPipeline.cc
    V4l2PipelineSrc.cc
    LibcameraPipelineSrc.cc
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
    V4l2PipelineSrc.h
    LibcameraPipelineSrc.h
    LibcameraHlsPipeline.h
    V4l2HlsPipeline.h
    CamService.h
)

set(LIBRARIES
    # Add libraries here
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})