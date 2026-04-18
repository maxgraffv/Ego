add_library(ccuda
    CCuda.cpp
    CudaKernels.cu
)


target_include_directories( ccuda PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(ccuda PUBLIC
    CUDA::cudart
)

set_target_properties(ccuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    CUDA_ARCHITECTURES 53
)