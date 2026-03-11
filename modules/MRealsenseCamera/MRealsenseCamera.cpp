#include "MRealsenseCamera.h"
#include <iostream>
#include <random>


MRealsenseCamera::MRealsenseCamera(Bus& bus, std::string bus_name) : AModule(bus, bus_name), _data(0.f)
{

}


void MRealsenseCamera::run()
{
    
    float fps = 5;
    int ms = static_cast<int>( 1000.f/fps );


    {
        // std::lock_guard(_data_mutex);
        _data = generateData();
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    Frame frame;
        frame.id = 0;
        frame.image = static_cast<int>(_data);
        frame.size = 1;
        frame.ts = 0;

    this->bus().publish("camera/frame", frame);

    std::cout << "MRealsenseCamera frame... " << static_cast<int>(_data) << std::endl;
}

double MRealsenseCamera::generateData()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 255.0);

    return dist(gen);
}