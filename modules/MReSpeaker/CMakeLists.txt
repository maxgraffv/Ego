add_library( respeaker
    MReSpeaker.cpp
)

target_include_directories(respeaker PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(respeaker PUBLIC
    amodule 
    asound
)