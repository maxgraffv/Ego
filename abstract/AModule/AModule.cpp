#include "AModule.h"


/**************************************************
 *	 STATIC
**************************************************/

/**
 * Static AModule ID Counter
 */
int AModule::_module_id_counter = 0;




/**************************************************
 *	 METHODS
**************************************************/

/**
 * Constructor
 */
AModule::AModule( Bus& bus, std::string bus_name ) : _stopRequested(false), _bus(bus), _bus_name(bus_name)
{
    this->_status = ModuleStatus::Inactive;
    this->_id = _module_id_counter++;
}


/**
 * Destructor
 */
AModule::~AModule()
{
    deactivate();
}


/**
 * Activate - starts module thread
 */
void AModule::activate()
{
    if (_status == ModuleStatus::Active) {
        return;
    }

    _stopRequested.store(false, std::memory_order_relaxed);
    this->_thread = std::thread(&AModule::_thread_run, this);
    this->_status = ModuleStatus::Active;
} 


/**
 * Deactivate - stops module thread
 */
void AModule::deactivate()
{
    if (!(_status == ModuleStatus::Active)) {
        return;
    }

    std::cout << "Called deactivate" << std::endl;
    _stopRequested.store(true, std::memory_order_relaxed);

    if (_thread.joinable()) {
        _thread.join();
    }

    this->_status = ModuleStatus::Inactive;
}


/**
 * Run
 */
void AModule::_thread_run()
{
    while(!_stopRequested.load(std::memory_order_relaxed)) {
        this->run();
    }
}


/**
 * Status
 */
ModuleStatus AModule::status()
{
    return _status;
}


/**
 * Bus
 */
Bus& AModule::bus()
{
    return _bus;
}


/**
 * Bus Name
 */
std::string& AModule::busName()
{
    return _bus_name;
}