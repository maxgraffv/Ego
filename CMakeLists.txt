cmake_minimum_required(VERSION 3.16)

project(Ego VERSION 0.1 LANGUAGES CXX C CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)

find_package(CUDAToolkit REQUIRED)

add_subdirectory(ITC)
add_subdirectory(modules/MCuda)
add_subdirectory(shared)
add_subdirectory(abstract/AModule)
add_subdirectory(abstract/APersonality)
add_subdirectory(modules/MRealsenseCamera)
add_subdirectory(modules/MQtDisplay)
add_subdirectory(personality/PRealsenseViewer)
add_subdirectory(core)

add_executable(ego main.cpp)
target_link_libraries(ego PRIVATE core)