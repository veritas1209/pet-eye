/**
 * PetEye ESP32-S3 Main File
 * Modular architecture for T-Camera S3
 * Version: 1.0.2
 */

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
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
void sendCameraSnapshot();

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
    
    // 초기 상태 로그
    DebugSystem::log("System ready - Camera: " + String(sysStatus.cameraInitialized ? "OK" : "FAIL") + 
                     ", Temp: " + String(sysStatus.tempSensorFound ? "OK" : "FAIL"));
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
    
    // 카메라 스냅샷 전송 (5초마다)
    static unsigned long lastCameraCapture = 0;
    if (sysStatus.wifiConnected && ENABLE_CAMERA && sysStatus.cameraInitialized) {
        if (millis() - lastCameraCapture > 5000) {  // 5초 간격
            lastCameraCapture = millis();
            sendCameraSnapshot();
        }
    }
    
    // 온도 데이터 전송 (10초마다 - 테스트용으로 빠르게 설정)
    static unsigned long lastApiSend = 0;
    if (sysStatus.wifiConnected && millis() - lastApiSend > 10000) {
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
        Serial.println("\n📊 Python Server Dashboard:");
        Serial.println("   http://192.168.0.10:5000");
        Serial.println("\n📡 API Base URL:");
        Serial.println("   " + String(API_BASE_URL));
    }
}

void sendDataToAPI() {
    if (!sysStatus.wifiConnected) {
        DebugSystem::log("Cannot send temperature - WiFi not connected");
        return;
    }
    
    HTTPClient http;
    String url = String(API_BASE_URL) + "/temperature";
    
    DebugSystem::log("Sending temperature to: " + url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(API_TIMEOUT);
    
    JsonDocument doc;
    doc["device_id"] = sysStatus.deviceId;
    doc["temperature"] = sysStatus.currentTemp;
    doc["timestamp"] = millis() / 1000;
    doc["rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    
    String jsonData;
    serializeJson(doc, jsonData);
    
    DebugSystem::log("Payload: " + jsonData);
    
    int httpCode = http.POST(jsonData);
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            DebugSystem::log("✅ Temperature sent: " + String(sysStatus.currentTemp, 1) + "°C");
        } else {
            DebugSystem::log("❌ HTTP error code: " + String(httpCode));
        }
    } else {
        DebugSystem::log("❌ HTTP POST failed: " + http.errorToString(httpCode));
    }
    
    http.end();
}

void sendCameraSnapshot() {
    if (!CameraManager::isInitialized()) {
        DebugSystem::log("Camera not initialized - skipping snapshot");
        return;
    }
    
    // 카메라 프레임 캡처
    camera_fb_t* fb = CameraManager::capture();
    if (!fb) {
        DebugSystem::log("❌ Failed to capture frame");
        return;
    }
    
    DebugSystem::log("📸 Captured frame: " + String(fb->len) + " bytes, " + 
                     String(fb->width) + "x" + String(fb->height));
    
    HTTPClient http;
    String url = String(API_BASE_URL) + "/upload";
    
    http.begin(url);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Device-ID", sysStatus.deviceId);
    http.addHeader("X-Timestamp", String(millis()));
    http.addHeader("X-Temperature", String(sysStatus.currentTemp, 1));
    http.addHeader("X-RSSI", String(WiFi.RSSI()));
    http.addHeader("X-Free-Heap", String(ESP.getFreeHeap()));
    http.setTimeout(15000);  // 15초 타임아웃 (이미지는 크므로)
    
    DebugSystem::log("Sending image to: " + url);
    
    // 바이너리 이미지 데이터 직접 전송
    int httpCode = http.POST(fb->buf, fb->len);
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            DebugSystem::log("✅ Image sent successfully");
            
            // 성공 시 LED 깜빡임 (옵션)
            pinMode(4, OUTPUT);  // 내장 LED (있는 경우)
            digitalWrite(4, HIGH);
            delay(100);
            digitalWrite(4, LOW);
        } else {
            DebugSystem::log("❌ Image upload failed - HTTP code: " + String(httpCode));
        }
    } else {
        DebugSystem::log("❌ Image POST failed: " + http.errorToString(httpCode));
    }
    
    http.end();
    CameraManager::releaseFrame(fb);
}