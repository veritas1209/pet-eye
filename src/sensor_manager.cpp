#include "sensor_manager.h"
#include <ArduinoJson.h>

SensorManager sensorManager;

SensorManager::SensorManager() : lastSensorRead(0), lastI2CScan(0) {}

bool SensorManager::init() {
    DEBUG_PRINTLN("\n--- 센서 초기화 시작 ---");
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    DEBUG_PRINTF("✓ I2C 초기화 완료 (SDA:%d, SCL:%d)\n", I2C_SDA, I2C_SCL);
    
    // 온도 센서 핀 설정
    pinMode(TEMP_SENSOR_PIN, INPUT);
    DEBUG_PRINTF("✓ 온도 센서 핀 설정 완료 (PIN:%d)\n", TEMP_SENSOR_PIN);
    
    // 센서 데이터 초기화
    sensorData.temperature = 0.0;
    sensorData.tempValid = false;
    sensorData.i2cDeviceCount = 0;
    sensorData.lastUpdate = 0;
    sensorData.isValid = false;
    
    // 초기 스캔 및 읽기
    scanI2CDevices();
    readTemperature();
    
    DEBUG_PRINTLN("✓ 센서 초기화 완료");
    return true;
}

void SensorManager::update() {
    unsigned long currentTime = millis();
    
    // 센서 데이터 업데이트
    if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
        readTemperature();
        lastSensorRead = currentTime;
    }
    
    // I2C 스캔 업데이트
    if (currentTime - lastI2CScan >= I2C_SCAN_INTERVAL) {
        scanI2CDevices();
        lastI2CScan = currentTime;
    }
}

void SensorManager::readTemperature() {
    int rawValue = analogRead(TEMP_SENSOR_PIN);
    float voltage = rawValue * (3.3 / 4095.0);
    
    // SEN050007 온도 센서 변환 공식 (데이터시트에 맞게 조정)
    sensorData.temperature = (voltage - 0.5) * 100.0;
    sensorData.tempValid = true;
    sensorData.lastUpdate = millis();
    sensorData.isValid = true;
    
    DEBUG_PRINTF("온도: Raw=%d, V=%.3f, T=%.2f°C\n", 
                 rawValue, voltage, sensorData.temperature);
}

void SensorManager::scanI2CDevices() {
    DEBUG_PRINTLN("I2C 장치 스캔 중...");
    sensorData.i2cDeviceCount = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            DEBUG_PRINTF("✓ I2C 장치 발견: 0x%02X\n", address);
            sensorData.i2cDeviceCount++;
        }
    }
    
    DEBUG_PRINTF("총 %d개의 I2C 장치 발견\n", sensorData.i2cDeviceCount);
}

void SensorManager::printSensorStatus() {
    DEBUG_PRINTLN("\n=== 센서 상태 ===");
    DEBUG_PRINTF("온도: %.2f°C (%s)\n", 
                 sensorData.temperature, 
                 sensorData.tempValid ? "유효" : "무효");
    DEBUG_PRINTF("I2C 장치: %d개\n", sensorData.i2cDeviceCount);
    DEBUG_PRINTF("마지막 업데이트: %lu ms 전\n", 
                 millis() - sensorData.lastUpdate);
    DEBUG_PRINTLN("================");
}

String SensorManager::getSensorJSON() {
    StaticJsonDocument<200> doc;
    doc["temperature"] = sensorData.temperature;
    doc["tempValid"] = sensorData.tempValid;
    doc["i2cDevices"] = sensorData.i2cDeviceCount;
    doc["lastUpdate"] = sensorData.lastUpdate;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}