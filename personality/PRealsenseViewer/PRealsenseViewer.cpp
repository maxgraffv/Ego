#include "PRealsenseViewer.h"


void PRealsenseViewer::run()
{
    ITC::Bus bus;
    MRealsenseCamera cam1(bus, "camera/rgbd");
    // MQtDisplay disp(bus, "camera/rgb");  // disabled: Qt not available on Jetson Nano

    auto bridge = bus.subscribe<FrameRGBD>("camera/rgbd", [&bus](const FrameRGBD& f) {
        FrameRGB rgb;
        rgb.id       = f.id;
        rgb.data     = f.rgb;
        rgb.channels = 3;
        rgb.width    = f.width;
        rgb.height   = f.height;
        rgb.ts       = f.ts;
        bus.publish<FrameRGB>("camera/rgb", rgb);
    });

    std::cout << "Activated" << std::endl;

    cam1.activate();
    // disp.activate();

    std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
    std::cout << "Done" << std::endl;

    cam1.deactivate();
    // disp.deactivate();
}
