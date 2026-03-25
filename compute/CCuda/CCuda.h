#ifndef M_CUDA_H
#define M_CUDA_H

#include <cstdint>
#include <vector>

class CCuda
{
public:
    CCuda();
    ~CCuda();

    // Input is image data in BGRBGR format
    // Returns new vector with increased contrast by 20%
    std::vector<uint8_t> applyContrast(const std::vector<uint8_t>& image) const;
};


#endif