#ifndef M_REALSENSE_CAMERA_H
#define M_REALSENSE_CAMERA_H

#include "AModule.h"
#include "Frame.h"
#include <librealsense2/rs.hpp>
#include <mutex>

class MRealsenseCamera : public AModule
{
    private:

        /**************************************************
         *	 IMAGING
         **************************************************/
        rs2::pipeline pipeline;
        rs2::config config;

        int wd = 1280;
        int hd = 720;
        int wc = 1920;
        int hc = 1080;

        double _data;

        rs2::align align_to_color = rs2::align(RS2_STREAM_COLOR);
        rs2::colorizer color_map;

        /**************************************************
         *	 HELPER FUNCTIONS
         **************************************************/
        double generateData();


    public:

        /**************************************************
         *	 CONSTRUCTORS
         **************************************************/
        MRealsenseCamera(ITC::Bus& bus, std::string bus_name);

        
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