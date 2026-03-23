#include "Core.h"
#include <iostream>
#include "Bus.h"
#include "MQtDisplay.h"   

EgoCore::EgoCore()
{

}

EgoCore::~EgoCore()
{

}

void EgoCore::start()
{
    ITC::Bus bus;
    MRealsenseCamera cam1(bus, "camera/frame");
    MQtDisplay disp(bus, "camera/frame");

    std::cout << "Activated" << std::endl;
    cam1.activate();
    disp.activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    std::cout << "Done" << std::endl;
    cam1.deactivate();
    disp.deactivate();
}