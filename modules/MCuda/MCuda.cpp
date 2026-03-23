#include "MCuda.h"

#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

// Funkcja zdefiniowana w CudaKernels.cu
void launchContrastKernel(uint8_t* d_image, std::size_t size);

namespace
{
    void checkCuda(cudaError_t err, const std::string& msg)
    {
        if (err != cudaSuccess)
        {
            throw std::runtime_error(msg + ": " + cudaGetErrorString(err));
        }
    }
}

MCuda::MCuda()
{
}

MCuda::~MCuda()
{
}

std::vector<uint8_t> MCuda::applyContrast(const std::vector<uint8_t>& image) const
{
    if (image.empty())
    {
        return {};
    }

    uint8_t* d_image = nullptr;
    const std::size_t sizeInBytes = image.size();

    checkCuda(cudaMalloc(&d_image, sizeInBytes), "cudaMalloc failed");

    try
    {
        checkCuda(
            cudaMemcpy(d_image, image.data(), sizeInBytes, cudaMemcpyHostToDevice),
            "cudaMemcpy HostToDevice failed"
        );

        launchContrastKernel(d_image, sizeInBytes);

        checkCuda(cudaDeviceSynchronize(), "cudaDeviceSynchronize failed");

        std::vector<uint8_t> result(sizeInBytes);

        checkCuda(
            cudaMemcpy(result.data(), d_image, sizeInBytes, cudaMemcpyDeviceToHost),
            "cudaMemcpy DeviceToHost failed"
        );

        cudaFree(d_image);
        return result;
    }
    catch (...)
    {
        if (d_image != nullptr)
        {
            cudaFree(d_image);
        }
        throw;
    }
}