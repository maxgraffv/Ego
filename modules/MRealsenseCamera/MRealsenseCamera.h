#ifndef M_REALSENSE_CAMERA_H
#define M_REALSENSE_CAMERA_H

#include "AModule.h"
#include "Frame.h"
#include <mutex>

class MRealsenseCamera : public AModule
{
    private:

        /**************************************************
         *	 IMAGING
         **************************************************/
        double _data;


        /**************************************************
         *	 HELPER FUNCTIONS
         **************************************************/
        double generateData();


    public:
        /**************************************************
         *	 CONSTRUCTORS
         **************************************************/
        MRealsenseCamera(Bus& bus, const std::string bus_name);

        
        /**************************************************
         *	 THREAD
         **************************************************/
        void run() override;

        
        /**************************************************
         *	 GETTERS
         **************************************************/
        double data();

};



#endif