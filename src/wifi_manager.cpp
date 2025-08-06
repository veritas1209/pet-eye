// src/wifi_manager.cpp - WiFi 연결 관리 구현
#include "wifi_manager.h"
#include "config.h"

void connectWiFi() {
    Serial.printf("WiFi 연결 시도: %s\n", WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("연결 중");
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d]", attempts);
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi 연결 성공!");
        Serial.printf("IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("신호 강도: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("❌ WiFi 연결 실패!");
        Serial.printf("상태 코드: %d\n", WiFi.status());
    }
}

void handleWiFiReconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
            Serial.println("WiFi 재연결 시도...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            lastReconnectAttempt = now;
        }
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}