#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "config.h"
#include "debug_system.h"

struct WiFiCredentials {
    char ssid[32];
    char password[64];
    bool valid;
};

class WiFiManager {
private:
    static WiFiCredentials credentials;
    static Preferences preferences;
    
public:
    static void init();
    static bool connect();
    static void startAP();
    static void loadCredentials();
    static void saveCredentials(const char* ssid, const char* password);
    static void clearCredentials();
    static bool isConnected();
    static void checkConnection();
    static String scanNetworks();
};

#endif // WIFI_MANAGER_H