#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "config.h"

class WebServerManager {
private:
    static WebServer server;
    
public:
    static void init();
    static void handle();
    
    // 페이지 핸들러
    static void handleRoot();
    static void handleDebugPage();
    static void handleScan();
    static void handleSave();
    static void handleStream();
    static void handleNotFound();
    
    // API 핸들러
    static void handleAPIDebug();
    static void handleAPIStatus();
    static void handleAPIClear();
    static void handleAPITestCamera();
    static void handleAPITestTemperature();
    static void handleAPITestAPI();
    static void handleAPIReboot();
};

#endif // WEB_SERVER_H