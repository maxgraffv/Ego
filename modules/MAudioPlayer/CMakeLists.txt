add_library(audio_player
    MAudioPlayer.cpp
)

target_include_directories(audio_player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(audio_player PUBLIC
    amodule
)
