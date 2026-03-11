#ifndef M_REALSENSE_CAMERA_H
#define M_REALSENSE_CAMERA_H

#include "AModule.h"
#include "Frame.h"
#include <mutex>

class MRealsenseCamera : public AModule
{
    private:

        std::mutex _data_mutex;
        double _data;

    public:
        MRealsenseCamera(Bus& bus, const std::string bus_name);

        void run() override;

        double generateData();
        double data();
};



#endif