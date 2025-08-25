/**
 * PetEye ESP32-S3 Main File
 * Modular architecture for T-Camera S3
 */

#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "debug_system.h"
#include "sensor_manager.h"
#include "camera_manager.h"

// System status
SystemStatus sysStatus;

// Function declarations
void initSystemStatus();
void printSystemInfo();
void sendDataToAPI();

void setup() {
    Serial.begin(115200);
    delay(2000);  // 시리얼 안정화
    
    // 시스템 초기화 배너
    Serial.println("\n\n");
    Serial.println("=====================================");
    Serial.println("    🐾 PetEye System Starting 🐾    ");
    Serial.println("=====================================");
    Serial.println("Firmware Version: " + String(FIRMWARE_VERSION));
    Serial.println("Board: LILYGO T-Camera ESP32-S3");
    
    // PSRAM 체크
    if(psramFound()){
        Serial.println("✅ PSRAM found: " + String(ESP.getPsramSize() / 1024) + " KB");
    } else {
        Serial.println("❌ No PSRAM found");
    }
    
    // 시스템 상태 초기화
    initSystemStatus();
    
    // 디버그 시스템 초기화
    DebugSystem::init();
    DebugSystem::log("System initialization started");
    
    // 센서 초기화
    SensorManager::init();
    
    // 카메라 초기화 (옵션)
    if (ENABLE_CAMERA) {
        CameraManager::init();
    }
    
    // WiFi Manager 초기화
    WiFiManager::init();
    
    // 웹 서버 시작
    WebServerManager::init();
    
    // 시스템 준비 완료
    Serial.println("\n=====================================");
    Serial.println("       🟢 System Ready! 🟢          ");
    Serial.println("=====================================");
    printSystemInfo();
    Serial.println("=====================================\n");
}

void loop() {
    // 웹 서버 처리
    WebServerManager::handle();
    
    // 센서 업데이트
    SensorManager::update();
    
    // WiFi 상태 체크 (30초마다)
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) {
        lastWiFiCheck = millis();
        WiFiManager::checkConnection();
    }
    
    // API 데이터 전송 (1분마다)
    static unsigned long lastApiSend = 0;
    if (sysStatus.wifiConnected && millis() - lastApiSend > 60000) {
        lastApiSend = millis();
        sendDataToAPI();
    }
    
    delay(10);
}

void initSystemStatus() {
    // MAC 주소에서 Device ID 생성
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceId[20];
    sprintf(deviceId, "PETEYE_%02X%02X%02X", mac[3], mac[4], mac[5]);
    sysStatus.deviceId = String(deviceId);
    
    Serial.println("Device ID: " + sysStatus.deviceId);
    Serial.println("=====================================\n");
    
    // 상태 초기화
    sysStatus.wifiConnected = false;
    sysStatus.cameraInitialized = false;
    sysStatus.tempSensorFound = false;
    sysStatus.mpuConnected = false;
    sysStatus.currentTemp = 0.0;
    sysStatus.lastTempRead = 0;
    sysStatus.lastApiUpdate = 0;
}

void printSystemInfo() {
    if (WiFi.getMode() == WIFI_AP) {
        Serial.println("📡 Access Point Mode:");
        Serial.println("   SSID: " + WiFi.softAPSSID());
        Serial.println("   Password: " + String(DEFAULT_AP_PASS));
        Serial.println("   IP: http://" + WiFi.softAPIP().toString());
        Serial.println("\n1. Connect to the WiFi network above");
        Serial.println("2. Open browser and go to IP address");
        Serial.println("3. Configure your home WiFi");
    } else {
        Serial.println("📶 Connected to WiFi:");
        Serial.println("   IP: http://" + WiFi.localIP().toString());
        Serial.println("   mDNS: http://" + String(DEVICE_NAME) + ".local");
    }
}

void sendDataToAPI() {
    // TODO: Implement API data sending
    DebugSystem::log("API update scheduled (temp: " + String(sysStatus.currentTemp, 1) + "°C)");
}