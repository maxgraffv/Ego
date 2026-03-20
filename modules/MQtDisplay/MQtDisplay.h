#ifndef M_QT_DISPLAY_H
#define M_QT_DISPLAY_H

#include "AModule.h"
#include "Bus.h"
#include "Frame.h"
#include "QtWindow.h"
#include <QApplication>
#include <mutex>
#include <iostream>
#include <optional>
#include <sys/types.h>
#include <sys/wait.h>


class MQtDisplay : public AModule
{
    private:

        /**************************************************
         *	 BUS
         **************************************************/
        ITC::Bus::Subscription _sub;
        Frame _last_frame;
        bool _has_new_frame = false;
        std::mutex _frame_mtx;


        /**************************************************
         *	 PROCESS
         **************************************************/
        pid_t _child_pid{-1};
        bool _process_open{false};
        void _run_QApp();

    public:

        /**************************************************
         *	 CONSTRUCTORS
         **************************************************/
        MQtDisplay(ITC::Bus& bus, const std::string bus_name);
        ~MQtDisplay();


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