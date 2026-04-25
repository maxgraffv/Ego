#include "PQBot.h"
#include "constants.h"

#include <memory>
#include <thread>

// Tune these for the physical robot
static constexpr int   kSpeed        = 400;
static constexpr int   kForwardMs    = 500;
static constexpr int   kTurnMs       = 200;
static constexpr int   kHelloEveryMs = 15000;


void PQBot::run()
{
    LOG("[PQBot] Starting...");

    ITC::Bus bus;
    LOG("[PQBot] ITC::Bus created");

    LOG("[PQBot] Initializing MAudioPlayer...");
    MAudioPlayer     player(bus, "audio/play");

    LOG("[PQBot] Initializing MMotorController...");
    MMotorController motors(bus, "motors/cmd");

    std::unique_ptr<MRealsenseCamera> cam;
    LOG("[PQBot] Attempting RealSense connection...");
    try {
        cam = std::make_unique<MRealsenseCamera>(bus, "camera/rgbd");
        LOG("[PQBot] RealSense: OK");
    } catch (const rs2::error& e) {
        LOG_ERR("[PQBot] RealSense unavailable: " << e.what() << " — continuing without camera");
    }

    auto rgbd_sub = bus.subscribe<FrameRGBD>("camera/rgbd",
        [this](const FrameRGBD& f) { onRGBD(f); });

    if (cam) {
        LOG("[PQBot] Activating camera...");
        cam->activate();
        LOG("[PQBot] Camera active");
    }

    LOG("[PQBot] Activating AudioPlayer...");
    player.activate();
    LOG("[PQBot] AudioPlayer active");

    LOG("[PQBot] Activating MotorController...");
    motors.activate();
    LOG("[PQBot] MotorController active");

    LOG("[PQBot] Playing hello()...");
    player.hello();
    auto last_hello = std::chrono::steady_clock::now();

    LOG("[PQBot] Entering main loop (square path)");

    int loop_count = 0;
    while (_running)
    {
        ++loop_count;
        LOG("[PQBot] Loop #" << loop_count << " — square run");

        for (int side = 0; side < 4 && _running; ++side)
        {
            auto now = std::chrono::steady_clock::now();
            int elapsed = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hello).count());

            if (elapsed >= kHelloEveryMs)
            {
                LOG("[PQBot] hello() (every " << kHelloEveryMs << "ms)");
                player.hello();
                last_hello = std::chrono::steady_clock::now();
            }

            LOG("[PQBot]   Side " << (side + 1) << "/4 — moving forward (" << kForwardMs << "ms)");
            motors.forward(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kForwardMs));

            LOG("[PQBot]   Side " << (side + 1) << "/4 — turning right (" << kTurnMs << "ms)");
            motors.turnRight(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kTurnMs));
        }

        LOG("[PQBot] Square completed");
    }

    LOG("[PQBot] Stopping motors — sending STOP to STM32");
    motors.stop();
}


void PQBot::stop()
{
    LOG("[PQBot] stop() — stopping loop");
    _running = false;
}

void PQBot::onRGBD(const FrameRGBD& frame)
{
    std::lock_guard<std::mutex> lock(_rgbd_mtx);
    _last_rgbd = frame;
}
