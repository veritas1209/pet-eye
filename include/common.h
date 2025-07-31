#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// 센서 데이터 구조체
struct SensorData {
    float temperature;
    float gyroX, gyroY, gyroZ;
    float accelX, accelY, accelZ;
    unsigned long timestamp;
    bool isValid;
};

// 전역 변수
extern SensorData currentSensorData;
extern WebServer server;

#endif