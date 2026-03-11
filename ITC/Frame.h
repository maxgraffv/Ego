#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <cstdint>
#include <ctime>

struct Frame {
    int id;
    uint8_t image;
    int size;
    time_t ts;
};


#endif