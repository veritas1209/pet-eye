// include/wifi_manager.h - WiFi 연결 관리 (구 버전 - wifi_config.h 사용 권장)
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "wifi_config.h"

// 호환성을 위한 래퍼 함수들
void connectWiFi();
void handleWiFiReconnection();
bool isWiFiConnected();

// 새 함수들 - wifi_config.h의 함수들을 사용하세요
// void initWiFiConfig();
// bool connectWithConfig(); 
// void startConfigPortal();

#endif