// src/main.cpp - 메인 애플리케이션 (리팩토링됨)
#include <Arduino.h>

// 각 모듈 헤더 포함
#include "config.h"
#include "sensor_data.h"
#include "wifi_manager.h"
#include "ds18b20_sensor.h"
#include "i2c_scanner.h"
#include "http_client.h"

void printSystemInfo() {
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
}

void printStatus() {
    static unsigned long lastStatus = 0;
    unsigned long now = millis();
    
    if (now - lastStatus >= STATUS_PRINT_INTERVAL) {
        String temp_status = sensors.temp_valid ? 
                           String(sensors.temperature, 2) + "°C" : "오류";
        String i2c_status = i2c_scan_complete ? String(sensors.i2c_count) : "스캔중";
        
        Serial.printf("[%lu] WiFi: %s | DS18B20: %s | I2C: %s | MEM: %dKB | 다음전송: %ds\n",
                      now / 1000,
                      isWiFiConnected() ? "연결됨" : "연결안됨",
                      temp_status.c_str(),
                      i2c_status.c_str(),
                      ESP.getFreeHeap() / 1024,
                      (SEND_INTERVAL - (now - sensors.last_send)) / 1000);
        lastStatus = now;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== T-Camera S3 + DS18B20 센서 시작 ===");
    printSystemInfo();
    
    // 데이터 구조체 초기화
    initSensorData();
    initDeviceInfo();
    
    // I2C 초기화
    initI2C();
    
    // DS18B20 초기화
    bool ds18b20_ok = initDS18B20();
    
    if (ds18b20_ok) {
        readTemperature(); // 첫 온도 읽기
        Serial.printf("초기 온도: %.2f°C\n", sensors.temperature);
    }
    
    Serial.println("I2C 스캔은 백그라운드에서 진행됩니다...");
    
    // WiFi 연결
    connectWiFi();
    
    Serial.printf("서버 URL: %s\n", SERVER_URL);
    Serial.printf("전송 간격: %d초\n", SEND_INTERVAL / 1000);
    Serial.println("=== 초기화 완료 ===\n");
}

void loop() {
    // WiFi 연결 상태 확인 및 재연결
    handleWiFiReconnection();
    
    // 센서 데이터 읽기
    readTemperature();
    scanI2C();
    
    // 서버에 데이터 전송 (주기적)
    unsigned long now = millis();
    if (sensors.data_ready && (now - sensors.last_send >= SEND_INTERVAL)) {
        if (sendSensorData()) {
            sensors.data_ready = false;
        }
        sensors.last_send = now;
    }
    
    // 상태 출력
    printStatus();
    
    delay(100);
}