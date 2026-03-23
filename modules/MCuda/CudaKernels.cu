#include <cuda_runtime.h>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace
{
    __device__ __forceinline__ uint8_t clampToByte(int value)
    {
        if (value < 0)   return 0;
        if (value > 255) return 255;
        return static_cast<uint8_t>(value);
    }

    __global__ void contrastKernel(uint8_t* image, std::size_t size)
    {
        const std::size_t idx = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;

        if (idx >= size)
        {
            return;
        }

        const float contrastFactor = 1.0f;

        const float oldValue = static_cast<float>(image[idx]);
        const float newValue = oldValue * contrastFactor;//(oldValue - 128.0f) * contrastFactor + 128.0f;

        image[idx] = clampToByte(static_cast<int>(newValue + 0.5f));
    }

    void checkCuda(cudaError_t err, const std::string& msg)
    {
        if (err != cudaSuccess)
        {
            throw std::runtime_error(msg + ": " + cudaGetErrorString(err));
        }
    }
}

void launchContrastKernel(uint8_t* d_image, std::size_t size)
{
    if (d_image == nullptr || size == 0)
    {
        return;
    }

    const int threadsPerBlock = 256;
    const int blocks = static_cast<int>((size + threadsPerBlock - 1) / threadsPerBlock);

    contrastKernel<<<blocks, threadsPerBlock>>>(d_image, size);

    checkCuda(cudaGetLastError(), "contrastKernel launch failed");
}