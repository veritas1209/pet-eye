// include/wifi_config.h - WiFi 설정 관리
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>

// WiFi 설정 구조체
struct WiFiCredentials {
    String ssid;
    String password;
    String server_url;
    bool is_configured;
};

// 설정 모드 상태
enum ConfigMode {
    CONFIG_MODE_NORMAL,     // 일반 모드
    CONFIG_MODE_AP,         // AP 모드 (설정 포털)
    CONFIG_MODE_SMARTCONFIG // SmartConfig 모드
};

// 전역 변수
extern WiFiCredentials wifi_config;
extern ConfigMode current_mode;
extern WebServer config_server;
extern DNSServer dns_server;
extern Preferences preferences;

// WiFi 설정 관리 함수들
void initWiFiConfig();
bool loadWiFiConfig();
void saveWiFiConfig();
void startConfigPortal();
void stopConfigPortal();
void handleConfigPortal();
bool connectWithConfig();
void startSmartConfig();
void handleSmartConfig();
void checkConfigButton();
void printWiFiConfig();

// 웹 핸들러 함수들
void handleRoot();
void handleSave();
void handleReset();
void handleStatus();

#endif