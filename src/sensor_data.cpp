// src/sensor_data.cpp - 센서 데이터 구조체 구현
#include "sensor_data.h"
#include <WiFi.h>

// 전역 변수 정의
SensorData sensors;
DeviceInfo device_info;

void initSensorData() {
    sensors.temperature = -999.0;
    sensors.temp_valid = false;
    sensors.i2c_count = 0;
    sensors.last_temp_read = 0;
    sensors.last_i2c_scan = 0;
    sensors.last_send = 0;
    sensors.data_ready = false;
    sensors.ds18b20_found = false;
    sensors.sensor_count = 0;
    
    Serial.println("센서 데이터 초기화 완료");
}

void initDeviceInfo() {
    device_info.device_id = WiFi.macAddress();
    device_info.chip_model = ESP.getChipModel();
    device_info.chip_revision = ESP.getChipRevision();
    device_info.cpu_freq = ESP.getCpuFreqMHz();
    device_info.mac_address = WiFi.macAddress();
    
    Serial.printf("디바이스 ID: %s\n", device_info.device_id.c_str());
    Serial.printf("Chip: %s Rev %d\n", device_info.chip_model.c_str(), device_info.chip_revision);
    Serial.printf("CPU: %d MHz\n", device_info.cpu_freq);
}