#include "MAudioPlayer.h"
#include "constants.h"

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>

static std::string audioAssetsPath()
{
    char buf[PATH_MAX] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len < 0) return "";
    std::string exe(buf, static_cast<std::size_t>(len));
    auto slash1 = exe.rfind('/');
    if (slash1 != std::string::npos) exe = exe.substr(0, slash1);
    auto slash2 = exe.rfind('/');
    if (slash2 != std::string::npos) exe = exe.substr(0, slash2);
    return exe + "/assets/audio/";
}

static bool fileExists(const std::string& path)
{
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}


/**************************************************
 *   CONSTRUCTORS
 **************************************************/

MAudioPlayer::MAudioPlayer(ITC::Bus& bus, std::string bus_name, std::string device)
    : AModule(bus, bus_name), _device(std::move(device))
{
    _sub = bus.subscribe<std::string>(bus_name, [this](const std::string& path) {
        play(path);
    });
}

MAudioPlayer::~MAudioPlayer()
{
    _sub.unsubscribe();
}


/**************************************************
 *   THREAD
 **************************************************/

void MAudioPlayer::run()
{
    std::string path;

    {
        std::lock_guard<std::mutex> lock(_queue_mtx);
        if (_queue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }
        path = _queue.front();
        _queue.pop();
    }

    auto dot = path.rfind('.');
    if (dot == std::string::npos)
    {
        std::cerr << "MAudioPlayer: unknown file format: " << path << "\n";
        return;
    }

    std::string ext = path.substr(dot);
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == ".mp3")
        playMp3(path);
    else
        playWav(path);
}


/**************************************************
 *   CONTROL
 **************************************************/

void MAudioPlayer::play(const std::string& path)
{
    std::lock_guard<std::mutex> lock(_queue_mtx);
    _queue.push(path);
}

void MAudioPlayer::hello()
{
    std::string path = audioAssetsPath() + "Hej_jestem_Q_robotic.wav";
    LOG("[MAudioPlayer] hello(): " << path);
    if (!fileExists(path))
    {
        LOG_ERR("[MAudioPlayer] hello(): plik nie znaleziony: " << path << " — pomijam");
        return;
    }
    play(path);
}


/**************************************************
 *   WAV PLAYBACK  (aplay)
 **************************************************/

void MAudioPlayer::playWav(const std::string& path)
{
    std::string cmd = "HOME=/tmp aplay -q \"" + path + "\"";
    LOG("[MAudioPlayer] playWav: " << cmd);
    int ret = system(cmd.c_str());
    if (ret != 0)
        std::cerr << "MAudioPlayer: aplay zwrócił kod " << ret << " dla: " << path << "\n";
}


/**************************************************
 *   MP3 PLAYBACK  (mpg123 command)
 **************************************************/

void MAudioPlayer::playMp3(const std::string& path)
{
    std::string cmd = "HOME=/tmp mpg123 -q \"" + path + "\"";
    LOG("[MAudioPlayer] playMp3: " << cmd);
    int ret = system(cmd.c_str());
    if (ret != 0)
        std::cerr << "MAudioPlayer: mpg123 zwrócił kod " << ret << " dla: " << path << "\n";
}
