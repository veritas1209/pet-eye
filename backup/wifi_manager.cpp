#include "wifi_manager.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager() : lastReconnectAttempt(0), isConnected(false) {}

bool WiFiManager::init() {
    DEBUG_PRINTLN("\n--- WiFi 초기화 시작 ---");
    DEBUG_PRINTF("SSID: %s\n", WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    DEBUG_PRINT("연결 중");
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            DEBUG_PRINTF(" [%d]", attempts);
        }
    }
    
    DEBUG_PRINTLN();
    
    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        DEBUG_PRINTLN("✓ WiFi 연결 성공!");
        printStatus();
        return true;
    } else {
        isConnected = false;
        DEBUG_PRINTLN("✗ WiFi 연결 실패!");
        DEBUG_PRINTF("상태 코드: %d\n", WiFi.status());
        return false;
    }
}

void WiFiManager::handleConnection() {
    if (WiFi.status() != WL_CONNECTED && isConnected) {
        isConnected = false;
        DEBUG_PRINTLN("WiFi 연결 끊어짐!");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
            DEBUG_PRINTLN("WiFi 재연결 시도...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            lastReconnectAttempt = currentTime;
        }
    } else if (!isConnected) {
        isConnected = true;
        DEBUG_PRINTLN("WiFi 재연결 성공!");
        printStatus();
    }
}

bool WiFiManager::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getConnectionInfo() {
    if (!isWiFiConnected()) {
        return "WiFi 연결 안됨";
    }
    
    String info = "SSID: " + WiFi.SSID();
    info += ", IP: " + WiFi.localIP().toString();
    info += ", RSSI: " + String(WiFi.RSSI()) + "dBm";
    return info;
}

void WiFiManager::printStatus() {
    if (isWiFiConnected()) {
        DEBUG_PRINTF("✓ SSID: %s\n", WiFi.SSID().c_str());
        DEBUG_PRINTF("✓ IP 주소: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTF("✓ 신호 강도: %d dBm\n", WiFi.RSSI());
        DEBUG_PRINTF("✓ Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        DEBUG_PRINTF("✓ DNS: %s\n", WiFi.dnsIP().toString().c_str());
    }
}