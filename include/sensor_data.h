// include/sensor_data.h - 센서 데이터 구조체 정의
#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

// 센서 데이터 구조체
struct SensorData {
    float temperature;
    bool temp_valid;
    int i2c_count;
    unsigned long last_temp_read;
    unsigned long last_i2c_scan;
    unsigned long last_send;
    bool data_ready;
    bool ds18b20_found;
    uint8_t sensor_count;
};

// 디바이스 정보 구조체
struct DeviceInfo {
    String device_id;
    String chip_model;
    int chip_revision;
    int cpu_freq;
    String mac_address;
};

// 전역 변수 선언
extern SensorData sensors;
extern DeviceInfo device_info;

// 초기화 함수들
void initSensorData();
void initDeviceInfo();

#endif