cmake_minimum_required(VERSION 3.5)
project(dashcam)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    utilities.cc
    RecordingPipeline.cc
    TsFileSinkPipeline.cc
    HlsPipelineSink.cc
    V4l2PipelineSrc.cc
    LibcameraPipelineSource.cc
    CamService.cc
    main.cc
)

set(HEADERS
    utilities.h
    RecordingPipeline.h
    TsFileSinkPipeline.h
    HlsPipelineSink.h
    V4l2PipelineSrc.h
    LibcameraPipelineSource.h
    CamService.h
)

set(LIBRARIES
    # Add libraries here
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})