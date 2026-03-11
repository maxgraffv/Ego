add_library(itc
    Frame.cpp
)

target_include_directories(itc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)