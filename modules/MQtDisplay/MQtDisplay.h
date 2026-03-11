#ifndef M_QT_DISPLAY_H
#define M_QT_DISPLAY_H

#include "AModule.h"
#include <mutex>
#include <iostream>
#include "Bus.h"
#include "Frame.h"

class MQtDisplay : public AModule
{
    private:
        Bus::Subscription _sub;
        Frame _last_frame;
        bool _has_new_frame = false;
        std::mutex _frame_mtx;

    public:
        MQtDisplay(Bus& bus, const std::string bus_name);

        void run() override;

        void render(const Frame& frame);
};



#endif