#ifndef A_MODULE_H
#define A_MODULE_H

#include "ModuleStatus.h"
#include "Bus.h"
#include <thread>
#include <atomic>
#include <iostream>

class AModule
{
    private:

        /**************************************************
         *	 STATIC
         **************************************************/
        static int _module_id_counter;


        /**************************************************
         *	 NONSTATIC
         **************************************************/
        int _id;
        std::atomic<ModuleStatus> _status;

        
        /**************************************************
         *  THREAD
         **************************************************/
        std::thread _thread;
        std::atomic<bool> _stopRequested;
        
        void _thread_run();
        virtual void run() = 0;
        void join();


        /**************************************************
         *	 BUS
         **************************************************/
        Bus& _bus;
        std::string _bus_name;


    public:

        /**
         * CONSTRUCTOR
         */
        AModule( Bus& bus, std::string bus_name );
        virtual ~AModule();


        /**************************************************
         *	 ACTIVATION
         **************************************************/
        void activate();
        void deactivate();


        /**************************************************
         *	 GETTERS
         **************************************************/
        ModuleStatus status();
        Bus& bus();
        std::string& busName();
};


#endif