add_library( amodule
    AModule.cpp
)

target_include_directories(amodule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(Threads REQUIRED)

target_link_libraries(amodule PUBLIC itc shared Threads::Threads)