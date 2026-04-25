#ifndef M_REALSENSE_CAMERA_H
#define M_REALSENSE_CAMERA_H

#include "AModule.h"
#include "FrameRGBD.h"
#include <librealsense2/rs.hpp>
#include <limits>
#include <mutex>
#include <vector>

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
         *   DEPTH CACHE
         *   Updated every run() tick, read by minDistance()
         **************************************************/
        std::mutex              _depth_mtx;
        std::vector<uint16_t>   _last_depth;

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

        // Returns the minimum valid depth in the latest frame, in metres.
        // Returns numeric_limits<float>::max() when no depth data is available.
        float minDistance();

};



#endif