add_library(mic_reader
    PMicReader.cpp
)

target_include_directories(mic_reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mic_reader PUBLIC
    apersonality
    itc
    respeaker
    qt_display
)