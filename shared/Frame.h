#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <cstdint>
#include <ctime>
#include <vector>

class Frame {
    public: 
        int id;
        std::vector<uint8_t> data;
        int channels;
        int length;
        int width;
        int height;
        time_t ts;

};


#endif
