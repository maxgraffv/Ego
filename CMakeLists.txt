cmake_minimum_required(VERSION 3.16)

project(Ego VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(ITC)
add_subdirectory(shared)
add_subdirectory(abstract/AModule)
add_subdirectory(modules/MRealsenseCamera)
add_subdirectory(modules/MQtDisplay)
add_subdirectory(core)

add_executable(ego main.cpp)
target_link_libraries(ego PRIVATE core)