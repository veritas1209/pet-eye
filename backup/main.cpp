#include "config.h"
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "web_server.h"

// 전역 변수 정의
SensorData sensorData;

void printStartupInfo() {
    Serial.println("\n=========================================");
    Serial.println("    T-Camera S3 Step 4: 모듈화된 센서 테스트");
    Serial.println("=========================================");
    
    Serial.printf("✓ Chip: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("✓ CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("✓ Flash: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
    Serial.printf("✓ Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("✓ PSRAM: %s\n", psramFound() ? "Available" : "Not found");
    
    if (psramFound()) {
        Serial.printf("✓ PSRAM Size: %d KB\n", ESP.getPsramSize() / 1024);
        Serial.printf("✓ Free PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
    }
    
    Serial.println("=========================================");
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    
    printStartupInfo();
    
    // 1. 센서 시스템 초기화
    if (!sensorManager.init()) {
        DEBUG_PRINTLN("❌ 센서 초기화 실패!");
    }
    
    // 2. WiFi 연결
    if (!wifiManager.init()) {
        DEBUG_PRINTLN("⚠️ WiFi 연결 실패 - 오프라인 모드");
    }
    
    // 3. 웹서버 시작
    if (wifiManager.isWiFiConnected()) {
        if (!webServerManager.init()) {
            DEBUG_PRINTLN("❌ 웹서버 시작 실패!");
        }
    }
    
    Serial.println("\n🚀 Step 4 모듈화된 시스템 초기화 완료!");
    Serial.println("=========================================\n");
}

void loop() {
    // 기존 코드...
    wifiManager.handleConnection();
    sensorManager.update();
    webServerManager.handleClient();
    
    // 웹서버 디버깅 추가
    static unsigned long lastWebTest = 0;
    if (millis() - lastWebTest > 5000) {
        if (wifiManager.isWiFiConnected()) {
            Serial.println("웹서버 상태: 정상 대기 중");
            Serial.printf("메모리 사용량: %d KB\n", ESP.getFreeHeap() / 1024);
        }
        lastWebTest = millis();
    }
    
    // 기존 상태 출력...
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus >= STATUS_PRINT_INTERVAL) {
        Serial.printf("[%lu] WiFi: %s, Heap: %d KB, Temp: %.2f°C, I2C: %d개\n",
                      millis() / 1000,
                      wifiManager.isWiFiConnected() ? "OK" : "FAIL", 
                      ESP.getFreeHeap() / 1024,
                      sensorData.temperature,
                      sensorData.i2cDeviceCount);
        lastStatus = millis();
    }
    
    delay(100);
}