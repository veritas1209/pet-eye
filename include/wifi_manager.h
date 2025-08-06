// include/wifi_manager.h - WiFi 연결 관리
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi 관리 함수들
void connectWiFi();
void handleWiFiReconnection();
bool isWiFiConnected();

#endif