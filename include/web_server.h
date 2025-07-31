#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "common.h"

class WebServerModule {
private:
    bool wifiConnected;
    void setupRoutes();

public:
    WebServerModule();
    bool initWiFi();
    void init();
    void handleClient();
    bool isWiFiConnected() const { return wifiConnected; }
    
    // 핸들러 함수들
    static void handleRoot();
    static void handleCapture();
    static void handleSensors();
    static void handleDataPage();
    static void handleNotFound();
    
    // HTML 템플릿 함수들
    static String getMainPageHTML();
    static String getDataPageHTML();
};

extern WebServerModule webServer;

#endif