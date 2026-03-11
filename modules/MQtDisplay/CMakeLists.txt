add_library(qt_display
    MQtDisplay.cpp
)

target_include_directories(qt_display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qt_display PUBLIC
    amodule
)