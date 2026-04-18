#ifndef M_QT_DISPLAY_H
#define M_QT_DISPLAY_H

#include "AModule.h"
#include "Bus.h"
#include "FrameRGB.h"
#include "SharedFrameIPC.h"
#include <QApplication>
#include <cstdint>
#include <mutex>
#include <iostream>
#include <optional>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>     // fork, _exit
#include <signal.h>     // kill

#include <fcntl.h>      // shm_open, O_*
#include <sys/eventfd.h>
#include <sys/mman.h>   // mmap, munmap, shm_open
#include <sys/stat.h>   // mode constants

class MQtDisplay : public AModule
{
    private:

        /**************************************************
         *	 BUS
         **************************************************/
        ITC::Bus::Subscription _sub;
        FrameRGB _last_frame;
        bool _has_new_frame = false;
        std::mutex _frame_mtx;


        /**************************************************
         *	 PROCESS
         **************************************************/
        pid_t _child_pid{-1};
        bool _process_open{false};

        /**************************************************
         *	 IPC
         **************************************************/
        const char* shm_name = SharedFrameIPC::kShmName;
        int fd{-1};
        int event_fd{-1};
        void* mem{nullptr};
        SharedFrameIPC::Header* header();
        std::uint8_t* payload();
        bool initIpc();
        void cleanupIpc();


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
        void render(const FrameRGB& frame);
};



#endif
