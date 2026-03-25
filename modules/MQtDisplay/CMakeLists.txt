set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(qt_display
    MQtDisplay.cpp
)

target_include_directories(qt_display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qt_display PUBLIC
    amodule 
    ccuda
    Qt6::Widgets
)

add_executable( qapp
    QtWindow.cpp
    VideoWidget.cpp
)

target_link_libraries(qapp PRIVATE
    Qt6::Widgets
)