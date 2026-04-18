#include "PQBot.h"

#include <iostream>
#include <memory>
#include <thread>

// Tune these for the physical robot
static constexpr int   kSpeed        = 200;
static constexpr int   kForwardMs    = 3000;   // straight leg of the square
static constexpr int   kTurnMs       = 1500;   // ~90° turn duration
static constexpr int   kHelloEveryMs = 15000;  // hello() interval (10–20s)


void PQBot::run()
{
    ITC::Bus bus;

    MAudioPlayer     player(bus, "audio/play");
    MMotorController motors(bus, "motors/cmd");

    std::unique_ptr<MRealsenseCamera> cam;
    try {
        cam = std::make_unique<MRealsenseCamera>(bus, "camera/rgbd");
    } catch (const rs2::error& e) {
        std::cerr << "[PQBot] RealSense not available: " << e.what() << std::endl;
    }

    auto rgbd_sub = bus.subscribe<FrameRGBD>("camera/rgbd",
        [this](const FrameRGBD& f) { onRGBD(f); });

    if (cam) cam->activate();
    player.activate();
    motors.activate();

    player.hello();
    auto last_hello = std::chrono::steady_clock::now();

    while (true)
    {
        // --- one side of the square ---
        for (int side = 0; side < 4; ++side)
        {
            // hello check
            auto now = std::chrono::steady_clock::now();
            int elapsed = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hello).count());

            if (elapsed >= kHelloEveryMs)
            {
                player.hello();
                last_hello = std::chrono::steady_clock::now();
            }

            // forward
            motors.forward(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kForwardMs));

            // turn right 90°
            motors.turnRight(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kTurnMs));
        }
    }
}


void PQBot::onRGBD(const FrameRGBD& frame)
{
    std::lock_guard<std::mutex> lock(_rgbd_mtx);
    _last_rgbd = frame;
}
