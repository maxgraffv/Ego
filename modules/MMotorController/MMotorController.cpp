#include "MMotorController.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>


/**************************************************
 *   CONSTRUCTORS
 **************************************************/

MMotorController::MMotorController(ITC::Bus& bus, std::string bus_name, std::string device)
    : AModule(bus, bus_name), _device(std::move(device))
{
    bus.subscribe<MotorCommand>(bus_name, [this](const MotorCommand& cmd) {
        enqueue(cmd);
    });
}

MMotorController::~MMotorController()
{
    closeSerial();
}


/**************************************************
 *   THREAD
 **************************************************/

void MMotorController::run()
{
    if (_fd < 0 && !openSerial())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return;
    }

    MotorCommand cmd;
    {
        std::lock_guard<std::mutex> lock(_queue_mtx);
        if (_queue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return;
        }
        cmd = _queue.front();
        _queue.pop();
    }

    dispatch(cmd);
}


/**************************************************
 *   DISPATCH
 **************************************************/

void MMotorController::dispatch(const MotorCommand& cmd)
{
    if (cmd.type == MotorCommand::Type::Stop)
    {
        _speed_l = _speed_r = 0;
        sendCommand("STOP\n");
        return;
    }

    int speed = std::clamp(cmd.speed, 0, 1000);
    int dir   = std::clamp(cmd.dir,   0, 1);

    if (cmd.motor == 1) { _speed_l = speed; _dir_l = dir; }
    else                { _speed_r = speed; _dir_r = dir; }

    std::string msg = "M" + std::to_string(cmd.motor) + " "
                          + std::to_string(speed)      + " "
                          + std::to_string(dir)        + "\n";
    sendCommand(msg);
}


/**************************************************
 *   CONTROL — per side
 **************************************************/

void MMotorController::setL(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, speed, _dir_l });
}

void MMotorController::setR(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 2, speed, _dir_r });
}

void MMotorController::setDirL(int dir)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, _speed_l, dir });
}

void MMotorController::setDirR(int dir)
{
    enqueue({ MotorCommand::Type::SetMotor, 2, _speed_r, dir });
}


/**************************************************
 *   CONTROL — movements
 **************************************************/

void MMotorController::stop()
{
    enqueue({ MotorCommand::Type::Stop });
}

void MMotorController::forward(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, speed, 1 });
    enqueue({ MotorCommand::Type::SetMotor, 2, speed, 1 });
}

void MMotorController::backward(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, speed, 0 });
    enqueue({ MotorCommand::Type::SetMotor, 2, speed, 0 });
}

void MMotorController::turnLeft(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, speed, 0 });
    enqueue({ MotorCommand::Type::SetMotor, 2, speed, 1 });
}

void MMotorController::turnRight(int speed)
{
    enqueue({ MotorCommand::Type::SetMotor, 1, speed, 1 });
    enqueue({ MotorCommand::Type::SetMotor, 2, speed, 0 });
}

void MMotorController::enqueue(MotorCommand cmd)
{
    std::lock_guard<std::mutex> lock(_queue_mtx);
    _queue.push(cmd);
}


/**************************************************
 *   SERIAL
 **************************************************/

bool MMotorController::openSerial()
{
    _fd = open(_device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (_fd < 0)
    {
        std::cerr << "MMotorController: cannot open " << _device
                  << ": " << std::strerror(errno) << "\n";
        return false;
    }

    termios tty{};
    tcgetattr(_fd, &tty);

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
    tty.c_iflag  = IGNBRK;
    tty.c_oflag  = 0;
    tty.c_lflag  = 0;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;  // 1s read timeout

    tcsetattr(_fd, TCSANOW, &tty);

    // STM32 USB CDC needs ~2s after enumeration
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    std::cout << "MMotorController: connected to " << _device << "\n";
    return true;
}

void MMotorController::closeSerial()
{
    if (_fd >= 0)
    {
        close(_fd);
        _fd = -1;
    }
}

bool MMotorController::sendCommand(const std::string& cmd)
{
    ssize_t written = write(_fd, cmd.c_str(), cmd.size());
    if (written < 0)
    {
        std::cerr << "MMotorController: write failed: " << std::strerror(errno) << "\n";
        closeSerial();
        return false;
    }

    // read all available bytes until timeout
    char buf[256] = {};
    std::string response;
    ssize_t n;
    while ((n = read(_fd, buf, sizeof(buf) - 1)) > 0)
    {
        buf[n] = '\0';
        response += buf;
    }

    if (!response.empty())
        std::cout << "MMotorController [" << _device << "] << " << response;

    if (response.find("ERR") != std::string::npos)
    {
        std::cerr << "MMotorController: STM32 ERR for command: " << cmd;
        return false;
    }

    return true;
}
