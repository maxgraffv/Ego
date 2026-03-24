#ifndef M_RE_SPEAKER_H
#define M_RE_SPEAKER_H

#include "AModule.h"

#include <alsa/asoundlib.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>


class MReSpeaker : public AModule
{
    private:

    

    public:

        /**************************************************
         *	 CONSTRUCTORS
         **************************************************/
        MReSpeaker(ITC::Bus& bus, std::string bus_name);

        
        /**************************************************
         *	 THREAD
         **************************************************/
        void run() override;


        /**************************************************
         *	 EXAMPLES
         **************************************************/
        void simple();
        void simpleSpectrum();

        /**************************************************
         *	 HELPER ALSA CHECK
         **************************************************/
        static void checkAlsa(int err, const char* what);
        
};



#endif