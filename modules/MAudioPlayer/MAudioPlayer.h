#ifndef M_AUDIO_PLAYER_H
#define M_AUDIO_PLAYER_H

#include "AModule.h"

#include <chrono>
#include <mutex>
#include <queue>
#include <string>
#include <thread>


class MAudioPlayer : public AModule
{
    private:

        /**************************************************
         *   BUS
         **************************************************/
        ITC::Bus::Subscription _sub;


        /**************************************************
         *   QUEUE
         **************************************************/
        std::queue<std::string> _queue;
        std::mutex _queue_mtx;


        /**************************************************
         *   PLAYBACK
         **************************************************/
        std::string _device;

        void playWav(const std::string& path);
        void playMp3(const std::string& path);


    public:

        /**************************************************
         *   CONSTRUCTORS
         **************************************************/
        MAudioPlayer(ITC::Bus& bus, std::string bus_name, std::string device = "default");
        ~MAudioPlayer();


        /**************************************************
         *   THREAD
         **************************************************/
        void run() override;


        /**************************************************
         *   CONTROL
         **************************************************/
        void play(const std::string& path);
        void hello();

};


#endif
