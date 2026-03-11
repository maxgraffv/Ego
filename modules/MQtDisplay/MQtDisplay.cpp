#include "MQtDisplay.h"


/**
 * Constructor
 */
MQtDisplay::MQtDisplay(Bus& bus, std::string bus_name) : AModule(bus, bus_name)
{
     _sub = this->bus().subscribe<Frame>(busName(),
        [this](const Frame& frame)
        {
            std::lock_guard<std::mutex> lock(_frame_mtx);
            _last_frame = frame;
            _has_new_frame = true;
        });
}


/**
 * Run
 */
void MQtDisplay::run()
{
    std::optional<Frame> frame_to_render;

    {
        std::lock_guard<std::mutex> lock(_frame_mtx);
        if (_has_new_frame)
        {
            frame_to_render = _last_frame;
            _has_new_frame = false;
        }
    }

    if (frame_to_render)
    {
        render(*frame_to_render);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


/**
 * Render
 */
void MQtDisplay::render( const Frame& frame)
{
    std::cout << "Rendering Frame " << static_cast<int>( frame.image ) << std::endl;
}
