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

    double ts = color.get_timestamp();
    const void* vdata = color.get_data();
    int vdata_len = color.get_data_size();

    int bitsperpix = color.get_bits_per_pixel();
    int bytesperpix = color.get_bytes_per_pixel();

    int frameW = color.get_width();
    int frameH = color.get_height();

    std::cout << "***************************" << std::endl;
    std::cout << "H x W: " << frameH << " x " << frameW << std::endl;
    std::cout << "Len: " << vdata_len << std::endl;
    std::cout << "Bytes: " << bytesperpix << std::endl;
    std::cout << "Bits: " << bitsperpix << std::endl;
    std::cout << "***************************\n" << std::endl;

    uint8_t *bytes = (uint8_t*) vdata;

    Frame frame;
        frame.id = 0;
        frame.image = bytes;
        frame.length = vdata_len;
        frame.height = frameH;
        frame.width = frameW;
        frame.ts = ts;

    this->bus().publish(busName(), frame);

    std::cout << "MRealsenseCamera frame... " << static_cast<int>(bytes[0]) << std::endl;
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