#include "MQtDisplay.h"


/**
 * Constructor
 */
MQtDisplay::MQtDisplay(ITC::Bus& bus, std::string bus_name) : AModule(bus, bus_name)
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
}


/**
 * Run
 */
void MQtDisplay::run()
{
    if(_process_open) {
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
    else
    {
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
            return;

        }
        else if ( pid == 0)
        {
            /**
             * QApplication - Child process
             */
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
void MQtDisplay::render( const Frame& frame)
{
    std::cout << "Rendering Frame " << static_cast<int>( frame.image[0] ) << std::endl;
}



void MQtDisplay::_run_QApp()
{

}