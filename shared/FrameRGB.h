#ifndef FRAME_RGB_H
#define FRAME_RGB_H

#include <cstdint>
#include <ctime>
#include <vector>

class FrameRGB {
    public:
        int id;
        std::vector<uint8_t> data;  // BGR8
        int channels;               // 3
        int width;
        int height;
        time_t ts;
};

#endif
