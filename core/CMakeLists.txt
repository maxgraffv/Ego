add_library(core
    Core.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC 
    apersonality
    realsense_viewer
)