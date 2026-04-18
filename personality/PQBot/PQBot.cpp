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
    std::cout << "[PQBot] Startuje..." << std::endl;

    ITC::Bus bus;
    std::cout << "[PQBot] ITC::Bus utworzony" << std::endl;

    std::cout << "[PQBot] Inicjalizacja MAudioPlayer..." << std::endl;
    MAudioPlayer     player(bus, "audio/play");

    std::cout << "[PQBot] Inicjalizacja MMotorController..." << std::endl;
    MMotorController motors(bus, "motors/cmd");

    std::unique_ptr<MRealsenseCamera> cam;
    std::cout << "[PQBot] Próba połączenia z RealSense..." << std::endl;
    try {
        cam = std::make_unique<MRealsenseCamera>(bus, "camera/rgbd");
        std::cout << "[PQBot] RealSense: OK" << std::endl;
    } catch (const rs2::error& e) {
        std::cerr << "[PQBot] RealSense niedostępny: " << e.what() << " — kontynuuję bez kamery" << std::endl;
    }

    auto rgbd_sub = bus.subscribe<FrameRGBD>("camera/rgbd",
        [this](const FrameRGBD& f) { onRGBD(f); });

    if (cam) {
        std::cout << "[PQBot] Aktywacja kamery..." << std::endl;
        cam->activate();
        std::cout << "[PQBot] Kamera aktywna" << std::endl;
    }

    std::cout << "[PQBot] Aktywacja AudioPlayer..." << std::endl;
    player.activate();
    std::cout << "[PQBot] AudioPlayer aktywny" << std::endl;

    std::cout << "[PQBot] Aktywacja MotorController..." << std::endl;
    motors.activate();
    std::cout << "[PQBot] MotorController aktywny" << std::endl;

    std::cout << "[PQBot] Odtwarzanie hello()..." << std::endl;
    player.hello();
    auto last_hello = std::chrono::steady_clock::now();

    std::cout << "[PQBot] Wchodzę w pętlę główną (kwadrat)" << std::endl;

    int loop_count = 0;
    while (true)
    {
        ++loop_count;
        std::cout << "[PQBot] Pętla #" << loop_count << " — przejazd kwadratu" << std::endl;

        for (int side = 0; side < 4; ++side)
        {
            auto now = std::chrono::steady_clock::now();
            int elapsed = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hello).count());

            if (elapsed >= kHelloEveryMs)
            {
                std::cout << "[PQBot] hello() (co " << kHelloEveryMs << "ms)" << std::endl;
                player.hello();
                last_hello = std::chrono::steady_clock::now();
            }

            std::cout << "[PQBot]   Bok " << (side + 1) << "/4 — jedzie do przodu (" << kForwardMs << "ms)" << std::endl;
            motors.forward(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kForwardMs));

            std::cout << "[PQBot]   Bok " << (side + 1) << "/4 — skręt w prawo (" << kTurnMs << "ms)" << std::endl;
            motors.turnRight(kSpeed);
            std::this_thread::sleep_for(std::chrono::milliseconds(kTurnMs));
        }

        std::cout << "[PQBot] Kwadrat ukończony" << std::endl;
    }
}


void PQBot::onRGBD(const FrameRGBD& frame)
{
    std::lock_guard<std::mutex> lock(_rgbd_mtx);
    _last_rgbd = frame;
}
