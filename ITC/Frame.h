#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <cstdint>
#include <ctime>

class Frame {
    public: 
        int id;
        uint8_t* image;
        int length;
        int width;
        int height;
        time_t ts;

};


#endif