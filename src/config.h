#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== VERSION ====================
#define FIRMWARE_VERSION "1.0.1"

// ==================== FEATURES ====================
#define ENABLE_CAMERA true  // 카메라 기능 켜기
#define ENABLE_TEMPERATURE true
#define ENABLE_MPU6050 false  // MPU6050 아직 미연결

// PMU
#define AXP2101_SLAVE_ADDRESS 0x34
#define PMU_SDA 7
#define PMU_SCL 6

// ==================== PIN DEFINITIONS (T-Camera S3) ====================
// Camera Data Pins (Y2-Y9)
#define Y2_GPIO_NUM     14
#define Y3_GPIO_NUM     47
#define Y4_GPIO_NUM     48
#define Y5_GPIO_NUM     21
#define Y6_GPIO_NUM     13
#define Y7_GPIO_NUM     11
#define Y8_GPIO_NUM     10
#define Y9_GPIO_NUM     9

// Camera Control Pins
#define XCLK_GPIO_NUM   38
#define PCLK_GPIO_NUM   12
#define VSYNC_GPIO_NUM  8
#define HREF_GPIO_NUM   18
#define SIOD_GPIO_NUM   5   // I2C SDA
#define SIOC_GPIO_NUM   4   // I2C SCL
#define PWDN_GPIO_NUM   -1  // Not used
#define RESET_GPIO_NUM  39

// JST 2.0mm 5PIN Connector
#define JST_IO16        16  // Data Pin 1
#define JST_IO15        15  // Data Pin 2

// Temperature Sensor (DS18B20)
#define TEMP_SENSOR_PIN JST_IO16  // 기본값 IO16

// MPU6050 Pins
#define MPU_SDA         3
#define MPU_SCL         45

// Other Peripherals
#define PIR_PIN         17
#define MIC_DOUT        41
#define MIC_SCLK        40
#define MIC_WS          42
#define BOOT_PIN        0

// PMU I2C (AXP2101)
#define PMU_SDA         7
#define PMU_SCL         6

// ==================== NETWORK CONFIGURATION ====================
#define DEFAULT_AP_SSID "PetEye-Config"
#define DEFAULT_AP_PASS "hajin1209"
#define DEVICE_NAME "PetEye"
#define WEB_SERVER_PORT 80
#define STREAM_SERVER_PORT 81

// ==================== API CONFIGURATION ====================
#define API_BASE_URL "http://192.168.0.10:5000/api"  // Python 서버 IP 주소
#define API_TIMEOUT 5000

// ==================== DEBUG CONFIGURATION ====================
#define DEBUG_BUFFER_SIZE 20
#define SERIAL_BAUD_RATE 115200

// ==================== SENSOR CONFIGURATION ====================
#define TEMP_READ_INTERVAL 5000  // 5초마다 온도 읽기
#define API_SEND_INTERVAL 10000  // 10초마다 API 전송 (테스트용)

// ==================== SYSTEM STATUS STRUCTURE ====================
struct SystemStatus {
    bool wifiConnected;
    bool cameraInitialized;
    bool tempSensorFound;
    bool mpuConnected;
    float currentTemp;
    unsigned long lastTempRead;
    unsigned long lastApiUpdate;
    String deviceId;
    IPAddress localIP;
};

// Global system status
extern SystemStatus sysStatus;

#endif // CONFIG_H