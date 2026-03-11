add_library(realsense_camera 
    MRealsenseCamera.cpp
)

target_include_directories(realsense_camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(realsense_camera PUBLIC
    amodule
)