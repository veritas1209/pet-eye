#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===== WiFi 설정 =====
#define WIFI_SSID "PRO"
#define WIFI_PASSWORD "propro123"
#define WIFI_TIMEOUT 30000  // 30초

// ===== 웹서버 설정 =====
#define WEB_SERVER_PORT 80

// ===== 센서 핀 설정 =====
#define I2C_SDA 17
#define I2C_SCL 18
#define TEMP_SENSOR_PIN 46

// ===== 타이밍 설정 =====
#define SENSOR_READ_INTERVAL 2000   // 2초
#define I2C_SCAN_INTERVAL 10000     // 10초
#define STATUS_PRINT_INTERVAL 10000 // 10초
#define WIFI_RECONNECT_INTERVAL 30000 // 30초

// ===== 디버그 설정 =====
#define DEBUG_ENABLED true
#define DEBUG_PRINT(x) if(DEBUG_ENABLED) Serial.print(x)
#define DEBUG_PRINTLN(x) if(DEBUG_ENABLED) Serial.println(x)
#define DEBUG_PRINTF(format, ...) if(DEBUG_ENABLED) Serial.printf(format, ##__VA_ARGS__)

// ===== 센서 데이터 구조체 =====
struct SensorData {
    float temperature;
    bool tempValid;
    int i2cDeviceCount;
    unsigned long lastUpdate;
    bool isValid;
};

// ===== 전역 변수 선언 =====
extern SensorData sensorData;

#endif