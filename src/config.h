#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== VERSION ====================
#define FIRMWARE_VERSION "1.0.1"

// ==================== FEATURES ====================
#define ENABLE_CAMERA false  // 카메라 기능 켜기/끄기
#define ENABLE_TEMPERATURE true
#define ENABLE_MPU6050 false  // MPU6050 아직 미연결

// ==================== PIN DEFINITIONS (T-Camera S3) ====================
// Camera Pins
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   39
#define CAM_PIN_XCLK    38
#define CAM_PIN_SIOD    5
#define CAM_PIN_SIOC    4

#define CAM_PIN_D7      11
#define CAM_PIN_D6      13
#define CAM_PIN_D5      21
#define CAM_PIN_D4      48
#define CAM_PIN_D3      47
#define CAM_PIN_D2      14
#define CAM_PIN_D1      10
#define CAM_PIN_D0      9
#define CAM_PIN_VSYNC   8
#define CAM_PIN_HREF    18
#define CAM_PIN_PCLK    12

// JST 2.0mm 5PIN Connector
#define JST_IO16        16  // Data Pin 1
#define JST_IO15        15  // Data Pin 2

// Temperature Sensor (DS18B20)
#define TEMP_SENSOR_PIN JST_IO16

// MPU6050 Pins
#define MPU_SDA         3
#define MPU_SCL         45

// Other Peripherals
#define PIR_PIN         17
#define MIC_DOUT        41
#define MIC_SCLK        40
#define MIC_WS          42
#define BOOT_PIN        0

// OLED Display
#define OLED_SDA        7
#define OLED_SCL        6

// ==================== NETWORK CONFIGURATION ====================
#define DEFAULT_AP_SSID "PetEye-Config"
#define DEFAULT_AP_PASS "12345678"
#define DEVICE_NAME "PetEye"
#define WEB_SERVER_PORT 80
#define STREAM_SERVER_PORT 81

// ==================== API CONFIGURATION ====================
#define API_BASE_URL "http://your-nextjs-server.com/api/pet-eye"
#define API_TIMEOUT 5000

// ==================== DEBUG CONFIGURATION ====================
#define DEBUG_BUFFER_SIZE 20
#define SERIAL_BAUD_RATE 115200

// ==================== SENSOR CONFIGURATION ====================
#define TEMP_READ_INTERVAL 5000  // 5초마다 온도 읽기
#define API_SEND_INTERVAL 60000  // 1분마다 API 전송

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