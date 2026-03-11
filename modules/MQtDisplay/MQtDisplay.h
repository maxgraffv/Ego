#ifndef M_QT_DISPLAY_H
#define M_QT_DISPLAY_H

#include "AModule.h"
#include "Bus.h"
#include "Frame.h"
#include <mutex>
#include <iostream>
#include <optional>

class MQtDisplay : public AModule
{
    private:

        /**************************************************
         *	 BUS
         **************************************************/
        Bus::Subscription _sub;
        Frame _last_frame;
        bool _has_new_frame = false;
        std::mutex _frame_mtx;


    public:

        /**************************************************
         *	 CONSTRUCTORS
         **************************************************/
        MQtDisplay(Bus& bus, const std::string bus_name);


        /**************************************************
         *	 THREAD
         **************************************************/
        void run() override;


        /**************************************************
         *	 GETTERS
         **************************************************/
        void render(const Frame& frame);
};



#endif