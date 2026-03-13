add_library(realsense_camera 
    MRealsenseCamera.cpp
)

find_package(realsense2 REQUIRED)

target_include_directories(realsense_camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(realsense_camera PUBLIC
    amodule realsense2::realsense2
)