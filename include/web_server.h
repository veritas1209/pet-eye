#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#include <WebServer.h>
#include <ArduinoJson.h>

class WebServerManager {
private:
    WebServer server;

public:
    WebServerManager();
    bool init();
    void handleClient();
    void setupRoutes();
    
    // 핸들러 함수들
    void handleRoot();
    void handleAPI();
    void handleSensors();
    void handleNotFound();
    
    // HTML 생성 함수들
    String generateMainHTML();
    String generateSystemInfoHTML();
    String generateNetworkInfoHTML();
};

extern WebServerManager webServerManager;

#endif