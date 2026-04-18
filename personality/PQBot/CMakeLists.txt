add_library(qbot
    PQBot.cpp
)

target_include_directories(qbot PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qbot PUBLIC
    apersonality
    itc
    realsense_camera
    audio_player
    motor_controller
)
