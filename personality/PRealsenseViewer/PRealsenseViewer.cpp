#include "PRealsenseViewer.h"


void PRealsenseViewer::run()
{
    ITC::Bus bus;
    MRealsenseCamera cam1(bus, "camera/frame");
    MQtDisplay disp(bus, "camera/frame");

    std::cout << "Activated" << std::endl;

    cam1.activate();
    disp.activate();

    std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
    std::cout << "Done" << std::endl;

    cam1.deactivate();
    disp.deactivate();
}