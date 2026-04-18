#ifndef M_RE_SPEAKER_H
#define M_RE_SPEAKER_H

#include "AModule.h"
#include "AudioFrame.h"

#include <alsa/asoundlib.h>
#include <cstdint>
#include <string>
#include <vector>


class MReSpeaker : public AModule
{
    private:

        /**************************************************
         *   CONFIG
         **************************************************/
        std::string _device;
        unsigned int _sample_rate   = 16000;
        unsigned int _num_channels  = 6;
        // 20ms @ 16kHz = 320 samples per channel
        static constexpr unsigned int kFrameSamples = 320;

        /**************************************************
         *   ALSA
         **************************************************/
        snd_pcm_t* _pcm = nullptr;

        bool openAlsa();
        void closeAlsa();
        static void checkAlsa(int err, const char* what);


    public:

        /**************************************************
         *   CONSTRUCTORS
         **************************************************/
        MReSpeaker(ITC::Bus& bus, std::string bus_name, std::string device = "hw:2,0");
        ~MReSpeaker();


        /**************************************************
         *   THREAD
         **************************************************/
        void run() override;

};


#endif
