add_library(realsense_viewer
    PRealsenseViewer.cpp
)

target_include_directories(realsense_viewer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(realsense_viewer PUBLIC
    apersonality
    itc
    # qt_display  # disabled: Qt not available on Jetson Nano
    realsense_camera
)