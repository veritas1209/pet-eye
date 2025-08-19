// src/wifi_manager.cpp - WiFi 연결 관리 구현 (호환성 래퍼)
#include "wifi_manager.h"
#include "wifi_config.h"

// 호환성을 위한 래퍼 함수들
void connectWiFi() {
    // 새로운 시스템 사용
    initWiFiConfig();
    connectWithConfig();
}

void handleWiFiReconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    // 설정 포털 처리
    handleConfigPortal();
    
    // SmartConfig 처리  
    handleSmartConfig();
    
    // 설정 버튼 체크
    checkConfigButton();
    
    if (WiFi.status() != WL_CONNECTED && wifi_config.is_configured) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= 30000) {
            Serial.println("WiFi 재연결 시도...");
            connectWithConfig();
            lastReconnectAttempt = now;
        }
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}