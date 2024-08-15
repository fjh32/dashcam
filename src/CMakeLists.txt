cmake_minimum_required(VERSION 3.5)
project(dashcam)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    utilities.cc
    GstRecordingPipeline.cc
    CamService.cc
    main.cc
)

set(HEADERS
    utilities.h
    RecordingPipeline.h
    CamService.h
)

set(LIBRARIES
    # Add libraries here
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})