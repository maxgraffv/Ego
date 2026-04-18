#include "MRealsenseCamera.h"
#include <iostream>
#include <random>


/**
 * Constructor
 */
MRealsenseCamera::MRealsenseCamera(ITC::Bus& bus, std::string bus_name) : AModule(bus, bus_name), _data(0.f)
{
    config.enable_stream(RS2_STREAM_COLOR, wc, hc, RS2_FORMAT_BGR8, 30);
    config.enable_stream(RS2_STREAM_DEPTH, wd, hd, RS2_FORMAT_Z16, 30);

    auto profile = pipeline.start(config);
}


/**
 * Run
 */
void MRealsenseCamera::run()
{
    _data = generateData();
    rs2::frameset fs = pipeline.wait_for_frames();
    fs = align_to_color.process(fs);

    rs2::video_frame color = fs.get_color_frame();
    rs2::depth_frame depth = fs.get_depth_frame();
    rs2::video_frame depth_vis = color_map.colorize(depth);

    double ts     = color.get_timestamp();
    const void* vdata = color.get_data();
    int vdata_len = color.get_data_size();
    int frameW    = color.get_width();
    int frameH    = color.get_height();

    const uint8_t* rgb_bytes  = reinterpret_cast<const uint8_t*>(vdata);
    const uint16_t* depth_bytes = reinterpret_cast<const uint16_t*>(depth.get_data());
    int depth_len = depth.get_data_size();

    FrameRGBD frame;
    frame.id     = 0;
    frame.width  = frameW;
    frame.height = frameH;
    frame.ts     = static_cast<time_t>(ts);
    frame.rgb    = std::vector<uint8_t>(rgb_bytes, rgb_bytes + vdata_len);
    frame.depth  = std::vector<uint16_t>(depth_bytes, depth_bytes + depth_len / sizeof(uint16_t));

    this->bus().publish(busName(), frame);
}


/**
 * Generate Random Data
 */
double MRealsenseCamera::generateData()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 255.0);

    return dist(gen);
}