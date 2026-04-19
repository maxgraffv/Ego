#include "PQBot.h"
#include "constants.h"

#include <memory>
#include <thread>

// Tune these for the physical robot
static constexpr int   kSpeed        = 200;
static constexpr int   kForwardMs    = 3000;
static constexpr int   kTurnMs       = 1500;
static constexpr int   kHelloEveryMs = 15000;


void PQBot::run()
{
    LOG("[PQBot] Startuje...");

    ITC::Bus bus;
    LOG("[PQBot] ITC::Bus utworzony");

    LOG("[PQBot] Inicjalizacja MAudioPlayer...");
    MAudioPlayer     player(bus, "audio/play");

    LOG("[PQBot] Inicjalizacja MMotorController...");
    MMotorController motors(bus, "motors/cmd");

    std::unique_ptr<MRealsenseCamera> cam;
    LOG("[PQBot] Próba połączenia z RealSense...");
    try {
        cam = std::make_unique<MRealsenseCamera>(bus, "camera/rgbd");
        LOG("[PQBot] RealSense: OK");
    } catch (const rs2::error& e) {
        LOG_ERR("[PQBot] RealSense niedostępny: " << e.what() << " — kontynuuję bez kamery");
    }

    auto rgbd_sub = bus.subscribe<FrameRGBD>("camera/rgbd",
        [this](const FrameRGBD& f) { onRGBD(f); });

    if (cam) {
        LOG("[PQBot] Aktywacja kamery...");
        cam->activate();
        LOG("[PQBot] Kamera aktywna");
    }

    LOG("[PQBot] Aktywacja AudioPlayer...");
    player.activate();
    LOG("[PQBot] AudioPlayer aktywny");

    LOG("[PQBot] Aktywacja MotorController...");
    motors.activate();
    LOG("[PQBot] MotorController aktywny");

    LOG("[PQBot] Odtwarzanie hello()...");
    player.hello();
    auto last_hello = std::chrono::steady_clock::now();

    LOG("[PQBot] Wchodzę w pętlę główną (kwadrat)");

    int loop_count = 0;
    while (true)
    {
        ++loop_count;
        LOG("[PQBot] Pętla #" << loop_count << " — przejazd kwadratu");

        for (int side = 0; side < 4; ++side)
        {
            auto now = std::chrono::steady_clock::now();
            int elapsed = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hello).count());

            if (elapsed >= kHelloEveryMs)
            {
                LOG("[PQBot] hello() (co " << kHelloEveryMs << "ms)");
                player.hello();
                last_hello = std::chrono::steady_clock::now();
            }

            LOG("[PQBot]   Bok " << (side + 1) << "/4 — jedzie do przodu (" << kForwardMs << "ms)");
            motors.forward(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kForwardMs));

            LOG("[PQBot]   Bok " << (side + 1) << "/4 — skręt w prawo (" << kTurnMs << "ms)");
            motors.turnRight(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kTurnMs));
        }

        LOG("[PQBot] Kwadrat ukończony");
    }
}


void PQBot::onRGBD(const FrameRGBD& frame)
{
    std::lock_guard<std::mutex> lock(_rgbd_mtx);
    _last_rgbd = frame;
}
