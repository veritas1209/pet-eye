#ifndef DEBUG_SYSTEM_H
#define DEBUG_SYSTEM_H

#include <Arduino.h>
#include "config.h"

class DebugSystem {
private:
    static String debugMessages[DEBUG_BUFFER_SIZE];
    static int debugIndex;
    
public:
    static void init();
    static void log(String message);
    static String getLog();
    static void clear();
};

#endif // DEBUG_SYSTEM_H