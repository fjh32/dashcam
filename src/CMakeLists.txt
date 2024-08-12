cmake_minimum_required(VERSION 3.5)
project(dashcam)

set(CMAKE_CXX_STANDARD 20)

# Add your source files here
set(SOURCES
    utilities.cc
    BluetoothServer.cc
    CamRecorder.cc
    CameraRecorder.cc
    GstRecordingPipeline.cc
    NetStreamServer.cc
    main.cc
    
    # Add more source files here
)

# Add your header files here
set(HEADERS
    utilities.h
    BluetoothServer.h
    CamRecorder.h
    CameraRecorder.h
    GstRecordingPipeline.h
    NetStreamServer.h
    # Add header files here
)

# Add any additional libraries or dependencies here
set(LIBRARIES
    # Add libraries here
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})