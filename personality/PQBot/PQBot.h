#ifndef P_QBOT_H
#define P_QBOT_H

#include "APersonality.h"
#include "Bus.h"
#include "FrameRGBD.h"
#include "MAudioPlayer.h"
#include "MMotorController.h"
#include "MRealsenseCamera.h"

#include <atomic>
#include <chrono>

class PQBot : public APersonality
{
    private:

        /**************************************************
         *   STATE
         **************************************************/
        std::atomic<bool>  _running{true};

        void turnRight(MMotorController& motors);


    public:
        void run() override;
        void stop() override;

};

#endif
