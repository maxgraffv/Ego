# Ego — Modular Robotics Platform

Ego is a C++17 framework for building autonomous robots. The goal is a platform for a physical mobile robot — capable of seeing its environment (RGBD camera), hearing (microphone array), responding with audio (audio player), and moving (motors). The framework provides ready-made hardware drivers and an architecture that lets you focus on robot behaviour rather than inter-thread communication.

Target platform: **NVIDIA Jetson Nano**.

---

## Architecture

### Structure

The platform is built from three concepts:

- **`EgoCore`** — the application entry point; holds a personality and manages its lifecycle
- **`APersonality`** — defines what the robot does; wires up modules and implements behaviour logic
- **`AModule`** — wraps a single piece of hardware; each module runs in its own thread

```
EgoCore
  └── APersonality        ← behaviour (what the robot does)
        ├── AModule        ← camera
        ├── AModule        ← microphone
        ├── AModule        ← motors
        └── ...
```

### Communication

Modules and personalities communicate through `ITC::Bus` — a type-safe publish/subscribe message bus. A module publishes data on a named topic; anything that subscribed to that topic receives it immediately in its own thread.

```
MRealsenseCamera  ──publish──▶  "camera/rgbd"  ──▶  PQBot (subscribed)
MReSpeaker        ──publish──▶  "mic/audio"    ──▶  PMicReader (subscribed)
```

Each module gets a reference to the bus and a topic name at construction time — that is its only communication channel.

Available modules: Intel RealSense camera (RGBD), ReSpeaker microphone array (6-channel, ALSA), audio player (WAV/MP3), motor controller (serial, STM32).

---

## Usage

Minimal program — pick a personality and run:

```cpp
#include "Core.h"
#include "PQBot.h"

int main()
{
    PQBot personality;
    EgoCore ego(personality);
    ego.start();   // blocks until stopped

    return 0;
}
```

The actual `main.cpp` adds signal handling (`SIGINT`, `SIGTERM`) so the robot shuts down cleanly on Ctrl+C:

```cpp
static EgoCore* g_ego = nullptr;

static void onSignal(int sig)
{
    if (g_ego) g_ego->stop();
    std::exit(0);
}

int main(int argc, char** argv)
{
    PQBot pqbot;
    EgoCore ego(pqbot);
    g_ego = &ego;

    std::signal(SIGTERM, onSignal);
    std::signal(SIGINT,  onSignal);

    ego.start();
    return 0;
}
```

To change the robot's behaviour, swap the personality class:

```cpp
PRealsenseViewer personality;   // camera viewer instead of PQBot
EgoCore ego(personality);
ego.start();
```

---

## Custom Module

Inherit from `AModule`, pass `Bus` and a topic name to the base constructor, and override `run()`.

`run()` is called **continuously in a loop** by a dedicated thread — block inside it (on I/O or sleep) rather than busy-waiting. The thread starts with `activate()` and stops with `deactivate()` (waits for join).

```cpp
#include "AModule.h"
#include <chrono>
#include <thread>

class MHeartbeat : public AModule
{
public:
    MHeartbeat(ITC::Bus& bus, std::string bus_name)
        : AModule(bus, bus_name) {}

private:
    void run() override
    {
        bus().publish<std::string>(busName(), "ping");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
};
```

---

## Custom Personality

```cpp
#include "APersonality.h"
#include "Bus.h"
#include "MMotorController.h"

class MyRobot : public APersonality
{
public:
    void run() override
    {
        ITC::Bus bus;

        MMotorController motors(bus, "motors");
        motors.activate();

        motors.forward(700);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        motors.stop();

        motors.deactivate();
    }
};
```

---

## Installation on NVIDIA Jetson

The repository includes `install_requirements.sh` which detects the platform and installs all dependencies automatically. On Jetson (aarch64) it builds librealsense2 from source with CUDA support — this takes ~20 minutes.

```bash
chmod +x install_requirements.sh
./install_requirements.sh
```

What the script installs:

| Step | Package |
|---|---|
| Build tools | `cmake`, `build-essential`, `git` |
| Audio | `libasound2-dev`, `libsndfile1-dev`, `libmpg123-dev` |
| Qt | Qt6 on Ubuntu 22.04+, Qt5 otherwise |
| RealSense | built from source on ARM, apt repo on x86 |
| CUDA | already included in JetPack — script skips it |

After the script finishes:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./ego
```

---

## Available Modules

| Module | Description |
|---|---|
| `MRealsenseCamera` | Intel RealSense RGBD camera — 1920×1080 color + 1280×720 depth, publishes `FrameRGBD` |
| `MReSpeaker` | ReSpeaker microphone array — 6-channel capture at 16 kHz via ALSA, publishes `AudioFrame` |
| `MAudioPlayer` | Audio playback — WAV and MP3, thread-safe queue, call `play(path)` from any thread |
| `MMotorController` | Dual motor controller over serial (STM32) — `forward()`, `backward()`, `turnLeft()`, `turnRight()`, `stop()` |
| `MQtDisplay` | Qt window for live RGB preview — uses shared memory IPC; disabled on Jetson Nano |

---

## Project Structure

```
Ego/
├── ITC/                # Pub/sub message bus
├── abstract/           # AModule, APersonality base classes
├── core/               # EgoCore
├── modules/            # Hardware drivers
├── personality/        # Built-in personalities
├── shared/             # Shared data types (FrameRGBD, AudioFrame, …)
├── compute/CCuda/      # CUDA utilities
├── install_requirements.sh
└── main.cpp
```
