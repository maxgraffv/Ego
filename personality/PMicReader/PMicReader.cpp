#include "PMicReader.h"


void PMicReader::run()
{
    ITC::Bus bus;
    MReSpeaker mic(bus, "audio/micsample");
    // MQtDisplay disp(bus, "camera/frame");

    std::cout << "Activated" << std::endl;

    mic.activate();
    // disp.activate();

    std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));
    std::cout << "Done" << std::endl;

    mic.deactivate();
    // disp.deactivate();
}