set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(QT_LIBS Qt6::Widgets)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    set(QT_LIBS Qt5::Widgets)
    message(STATUS "Using Qt5")
endif()

add_library(qt_display
    MQtDisplay.cpp
)

target_include_directories(qt_display PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qt_display PUBLIC
    amodule
    ccuda
    ${QT_LIBS}
)

add_executable(qapp
    QtWindow.cpp
    VideoWidget.cpp
)

target_link_libraries(qapp PRIVATE
    ${QT_LIBS}
)
