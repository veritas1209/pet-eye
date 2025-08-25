#include "sensor_manager.h"

OneWire SensorManager::oneWire(TEMP_SENSOR_PIN);
DallasTemperature SensorManager::tempSensor(&oneWire);

void SensorManager::init() {
    if (ENABLE_TEMPERATURE) {
        DebugSystem::log("Initializing temperature sensor...");
        tempSensor.begin();
        
        if (tempSensor.getDeviceCount() > 0) {
            sysStatus.tempSensorFound = true;
            tempSensor.setResolution(12);
            DebugSystem::log("✅ Temperature sensor found");
        } else {
            sysStatus.tempSensorFound = false;
            DebugSystem::log("❌ Temperature sensor not found");
        }
    }
    
    if (ENABLE_MPU6050) {
        // TODO: MPU6050 초기화
        DebugSystem::log("MPU6050 not yet implemented");
    }
}

void SensorManager::update() {
    // 온도 센서 업데이트
    if (ENABLE_TEMPERATURE && sysStatus.tempSensorFound) {
        if (millis() - sysStatus.lastTempRead > TEMP_READ_INTERVAL) {
            float temp = readTemperature();
            if (temp != DEVICE_DISCONNECTED_C) {
                sysStatus.currentTemp = temp;
                sysStatus.lastTempRead = millis();
                
                // 온도 변화가 1도 이상일 때만 로그
                static float lastLoggedTemp = 0;
                if (abs(temp - lastLoggedTemp) > 1.0) {
                    DebugSystem::log("Temperature: " + String(temp, 1) + "°C");
                    lastLoggedTemp = temp;
                }
            }
        }
    }
}

float SensorManager::readTemperature() {
    if (!sysStatus.tempSensorFound) {
        return DEVICE_DISCONNECTED_C;
    }
    
    tempSensor.requestTemperatures();
    return tempSensor.getTempCByIndex(0);
}

bool SensorManager::isTemperatureSensorConnected() {
    return sysStatus.tempSensorFound;
}