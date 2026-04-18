#ifndef M_MOTOR_CONTROLLER_H
#define M_MOTOR_CONTROLLER_H

#include "AModule.h"

#include <mutex>
#include <queue>
#include <string>


struct MotorCommand
{
    enum class Type { SetMotor, Stop };

    Type type  = Type::Stop;
    int motor  = 1;    // 1 or 2
    int speed  = 0;    // 0–1000
    int dir    = 0;    // 0 = backward, 1 = forward
};


class MMotorController : public AModule
{
    private:

        /**************************************************
         *   SERIAL
         **************************************************/
        std::string _device;
        int _fd = -1;

        bool openSerial();
        void closeSerial();
        bool sendCommand(const std::string& cmd);


        /**************************************************
         *   QUEUE
         **************************************************/
        std::queue<MotorCommand> _queue;
        std::mutex _queue_mtx;

        void enqueue(MotorCommand cmd);
        void dispatch(const MotorCommand& cmd);


        /**************************************************
         *   MOTOR STATE
         *   STM32 requires speed+dir together — we track
         *   current values so setL/setDirL can fill the gap
         **************************************************/
        int _speed_l = 0;   int _dir_l = 1;
        int _speed_r = 0;   int _dir_r = 1;


    public:

        /**************************************************
         *   CONSTRUCTORS
         **************************************************/
        MMotorController(ITC::Bus& bus, std::string bus_name,
                         std::string device = "/dev/ttyACM0");
        ~MMotorController();


        /**************************************************
         *   THREAD
         **************************************************/
        void run() override;


        /**************************************************
         *   CONTROL — per side
         **************************************************/
        void setL(int speed);
        void setR(int speed);
        void setDirL(int dir);
        void setDirR(int dir);


        /**************************************************
         *   CONTROL — movements
         **************************************************/
        void stop();
        void forward(int speed = 500);
        void backward(int speed = 500);
        void turnLeft(int speed = 500);
        void turnRight(int speed = 500);

};


#endif
