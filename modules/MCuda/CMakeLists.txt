add_library(mcuda
    MCuda.cpp
    CudaKernels.cu
)


target_include_directories( mcuda PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(mcuda PUBLIC
    CUDA::cudart
)

set_target_properties(mcuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)