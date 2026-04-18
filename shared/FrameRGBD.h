#ifndef FRAME_RGBD_H
#define FRAME_RGBD_H

#include <cstdint>
#include <ctime>
#include <vector>

class FrameRGBD {
    public:
        int id;
        std::vector<uint8_t> rgb;    // BGR8, width x height x 3
        std::vector<uint16_t> depth; // Z16, aligned to color resolution
        int width;
        int height;
        time_t ts;
};

#endif
