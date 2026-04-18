#ifndef P_QBOT_H
#define P_QBOT_H

#include "APersonality.h"
#include "Bus.h"
#include "FrameRGBD.h"
#include "MAudioPlayer.h"
#include "MMotorController.h"
#include "MRealsenseCamera.h"

#include <chrono>
#include <mutex>

class PQBot : public APersonality
{
    private:

        /**************************************************
         *   STATE
         **************************************************/
        FrameRGBD  _last_rgbd;
        std::mutex _rgbd_mtx;

        void onRGBD(const FrameRGBD& frame);


    public:
        void run() override;

};

#endif
