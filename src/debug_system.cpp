#include "debug_system.h"

String DebugSystem::debugMessages[DEBUG_BUFFER_SIZE];
int DebugSystem::debugIndex = 0;

void DebugSystem::init() {
    for (int i = 0; i < DEBUG_BUFFER_SIZE; i++) {
        debugMessages[i] = "";
    }
    log("Debug system initialized");
}

void DebugSystem::log(String message) {
    String timestamp = String(millis() / 1000) + "s: ";
    String fullMessage = timestamp + message;
    
    Serial.println(fullMessage);
    
    debugMessages[debugIndex] = fullMessage;
    debugIndex = (debugIndex + 1) % DEBUG_BUFFER_SIZE;
}

String DebugSystem::getLog() {
    String log = "";
    for (int i = 0; i < DEBUG_BUFFER_SIZE; i++) {
        int idx = (debugIndex + i) % DEBUG_BUFFER_SIZE;
        if (debugMessages[idx].length() > 0) {
            log += debugMessages[idx] + "\n";
        }
    }
    return log;
}

void DebugSystem::clear() {
    for (int i = 0; i < DEBUG_BUFFER_SIZE; i++) {
        debugMessages[i] = "";
    }
    debugIndex = 0;
    log("Debug log cleared");
}