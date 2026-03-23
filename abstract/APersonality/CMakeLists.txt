add_library( apersonality
    DefaultPersonality.cpp
)

target_include_directories(apersonality PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)