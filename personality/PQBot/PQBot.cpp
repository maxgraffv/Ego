#include "PQBot.h"
#include "constants.h"

#include <limits>
#include <memory>
#include <thread>

static constexpr int   kSpeed        = 400;
static constexpr int   kTurnMs       = 300;   // ms per right-turn burst
static constexpr int   kPollMs       = 50;    // ms between distance checks while driving
static constexpr int   kHelloEveryMs = 15000;
static constexpr float kObstacleM    = 0.25f; // stop and turn when closer than this


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

    LOG("[PQBot] Entering main loop (obstacle avoidance)");

    while (_running)
    {
        auto now = std::chrono::steady_clock::now();
        int elapsed = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hello).count());
        if (elapsed >= kHelloEveryMs)
        {
            player.hello();
            last_hello = std::chrono::steady_clock::now();
        }

        float dist = cam ? cam->minDistance() : std::numeric_limits<float>::max();
        LOG("[PQBot] min distance: " << dist << "m");

        if (dist <= kObstacleM)
            turnRight(motors);
        else
            motors.forward(kSpeed);

        std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
    }

    LOG("[PQBot] Stopping motors — sending STOP to STM32");
    motors.stop();
}


void PQBot::turnRight(MMotorController& motors)
{
    LOG("[PQBot] Obstacle detected — turning right");
    motors.turnRight(kSpeed);
    std::this_thread::sleep_for(std::chrono::milliseconds(kTurnMs));
}


void PQBot::stop()
{
    LOG("[PQBot] stop() — stopping loop");
    _running = false;
}

