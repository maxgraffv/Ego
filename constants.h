#ifndef EGO_CONSTANTS_H
#define EGO_CONSTANTS_H

#include <iostream>

// Set to 1 to enable debug logging, 0 to disable
#define DEBUG_FLAG 1

#if DEBUG_FLAG
#define LOG(msg) std::cout << msg << std::endl
#define LOG_ERR(msg) std::cerr << msg << std::endl
#else
#define LOG(msg)
#define LOG_ERR(msg)
#endif

#endif
