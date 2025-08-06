// include/config.h - 전역 설정
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// WiFi 설정
#define WIFI_SSID "PRO"
#define WIFI_PASSWORD "propro123"
#define WIFI_TIMEOUT 30000
#define WIFI_RECONNECT_INTERVAL 30000

// 서버 설정
#define SERVER_URL "http://192.168.0.27:3000/api/sensors"
#define SEND_INTERVAL 10000  // 10초마다 전송
#define HTTP_TIMEOUT 5000

// 핀 설정
#define ONE_WIRE_BUS 3    // DS18B20 데이터 핀
#define I2C_SDA 17         // I2C SDA 핀
#define I2C_SCL 18         // I2C SCL 핀

// 센서 읽기 간격
#define TEMP_READ_INTERVAL 2000    // 2초마다 온도 읽기
#define I2C_SCAN_INTERVAL 300000   // 5분마다 I2C 재스캔
#define STATUS_PRINT_INTERVAL 5000 // 5초마다 상태 출력

// I2C 설정
#define I2C_FREQ 400000
#define I2C_TIMEOUT 5
#define I2C_SCAN_DELAY 10

// 디바이스 정보
#define DEVICE_NAME "T-Camera-S3-DS18B20"
#define DEVICE_LOCATION "Lab-01"

#endif