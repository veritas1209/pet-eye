#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>

class WiFiManager {
private:
    unsigned long lastReconnectAttempt;
    bool isConnected;

public:
    WiFiManager();
    bool init();
    void handleConnection();
    bool isWiFiConnected() const;
    String getConnectionInfo();
    void printStatus();
};

extern WiFiManager wifiManager;

#endif