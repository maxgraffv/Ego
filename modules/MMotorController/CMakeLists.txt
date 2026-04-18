add_library(motor_controller
    MMotorController.cpp
)

target_include_directories(motor_controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(motor_controller PUBLIC
    amodule
)
