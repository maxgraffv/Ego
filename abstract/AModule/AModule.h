#ifndef A_MODULE_H
#define A_MODULE_H

#include "ModuleStatus.h"
#include "Bus.h"
#include <thread>
#include <atomic>

class AModule
{
    private:
        std::atomic<bool> _stopRequested;
        std::atomic<ModuleStatus> _status;
        Bus& _bus;
        std::string _bus_name;

        static int _module_id_counter;
        std::thread _thread;

        void _thread_run();
        virtual void run() = 0;

        void join();

    public:
        AModule( Bus& bus, std::string bus_name );
        virtual ~AModule();

        void activate();
        void deactivate();

        Bus& bus();
        std::string& busName();

        ModuleStatus status();
};





#endif