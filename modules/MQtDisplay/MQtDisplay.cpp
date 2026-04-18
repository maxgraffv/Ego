#include "MQtDisplay.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <string>

#include "CCuda.h"


/**
 * Constructor
 */
MQtDisplay::MQtDisplay(ITC::Bus& bus, std::string bus_name) : AModule(bus, bus_name)
{
    _sub = this->bus().subscribe<FrameRGB>(busName(),
            [this](const FrameRGB& frame)
            {
                std::lock_guard<std::mutex> lock(_frame_mtx);
                _last_frame = frame;
                _has_new_frame = true;
            });
}


/**
 * Destructor
 */
MQtDisplay::~MQtDisplay()
{
    if (_child_pid > 0)
    {
        std::cout << "Child Process killed" << _child_pid << std::endl;
        kill(_child_pid, SIGTERM);
        waitpid(_child_pid, nullptr, 0);
    }

    cleanupIpc();
}


/**
 * Run
 */
void MQtDisplay::run()
{
    if(_process_open) {
        std::optional<FrameRGB> frame_to_render;

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
            /*
                TO INVESTIGATE !!!!
                is this render() only called in the Parent process 
                                                or also a child process ?????
            */
            render(*frame_to_render);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    else
    {
        if (!initIpc())
        {
            return;
        }

        pid_t pid = fork(); 
        if (pid < 0)
        {
            /**
             * Fork failed
             */
            /*
                To Investigate !!!!!
                parent _worker_thread() Doesn't support try/catch at the moment
            */
            // throw std::runtime_error("QtDisplay fork() failed");
            std::cerr << "fork() failed\n";
            cleanupIpc();
            return;

        }
        else if ( pid == 0)
        {
            /**
             * QApplication - Child process
             */
            const std::string event_fd_text = std::to_string(event_fd);
            setenv(SharedFrameIPC::kEventFdEnv, event_fd_text.c_str(), 1);
            setenv(SharedFrameIPC::kShmNameEnv, shm_name, 1);
            execl("./modules/MQtDisplay/qapp", "./modules/MQtDisplay/qapp", (char*)nullptr);
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != nullptr)
            {
                std::cout << "Child cwd: " << cwd << std::endl;
            }
            else
            {
                perror("getcwd");
            }

            // if execl returns then an error occured
            std::cerr << "execl() failed\n";
            _exit(1);
        } 
        else
        {
            /**
             * MQtDisplay - Parent process (Ego thread)
             */
            _child_pid = pid;
            _process_open = true;
            return;
        }
    }

}


/**
 * Render
 */
void MQtDisplay::render( const FrameRGB& frame)
{
    if (mem == nullptr || frame.data.empty())
    {
        return;
    }

    if (frame.data.size() > SharedFrameIPC::kMaxFrameBytes)
    {
        std::cerr << "FrameRGB too large for shared memory buffer: " << frame.data.size() << std::endl;
        return;
    }

    SharedFrameIPC::Header* meta = header();

    std::memcpy(payload(), frame.data.data(), frame.data.size());
    meta->width = static_cast<std::uint32_t>(frame.width);
    meta->height = static_cast<std::uint32_t>(frame.height);
    meta->channels = static_cast<std::uint32_t>(frame.channels);
    meta->size = static_cast<std::uint32_t>(frame.data.size());
    ++meta->sequence;

    const std::uint64_t signal_value = 1;
    const ssize_t write_result = write(event_fd, &signal_value, sizeof(signal_value));
    if (write_result != sizeof(signal_value))
    {
        std::cerr << "eventfd write failed: " << std::strerror(errno) << std::endl;
    }
}

SharedFrameIPC::Header* MQtDisplay::header()
{
    return static_cast<SharedFrameIPC::Header*>(mem);
}

std::uint8_t* MQtDisplay::payload()
{
    return static_cast<std::uint8_t*>(mem) + sizeof(SharedFrameIPC::Header);
}

bool MQtDisplay::initIpc()
{
    fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1)
    {
        std::cerr << "shm_open failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    if (ftruncate(fd, SharedFrameIPC::kMappingSize) == -1)
    {
        std::cerr << "ftruncate failed: " << std::strerror(errno) << std::endl;
        cleanupIpc();
        return false;
    }

    mem = mmap(nullptr,
               SharedFrameIPC::kMappingSize,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               0);

    if (mem == MAP_FAILED)
    {
        mem = nullptr;
        std::cerr << "mmap failed: " << std::strerror(errno) << std::endl;
        cleanupIpc();
        return false;
    }

    event_fd = eventfd(0, 0);
    if (event_fd == -1)
    {
        std::cerr << "eventfd failed: " << std::strerror(errno) << std::endl;
        cleanupIpc();
        return false;
    }

    SharedFrameIPC::Header* meta = header();
    meta->sequence = 0;
    meta->width = 0;
    meta->height = 0;
    meta->channels = 0;
    meta->size = 0;
    return true;
}

void MQtDisplay::cleanupIpc()
{
    if (mem != nullptr)
    {
        munmap(mem, SharedFrameIPC::kMappingSize);
        mem = nullptr;
    }

    if (fd != -1)
    {
        close(fd);
        fd = -1;
        shm_unlink(shm_name);
    }

    if (event_fd != -1)
    {
        close(event_fd);
        event_fd = -1;
    }
}
