
#ifndef CONFIG_H
#define CONFIG_H

// WiFi 설정
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// 서버 설정
#define WEB_SERVER_PORT 80

// T-Camera S3 카메라 핀 정의
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     40
#define SIOD_GPIO_NUM     17
#define SIOC_GPIO_NUM     18
#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       41
#define Y7_GPIO_NUM       42
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       3
#define Y4_GPIO_NUM       14
#define Y3_GPIO_NUM       47
#define Y2_GPIO_NUM       13
#define VSYNC_GPIO_NUM    21
#define HREF_GPIO_NUM     38
#define PCLK_GPIO_NUM     11

// 센서 핀 정의
#define TEMP_SENSOR_PIN   4
#define I2C_SDA           17
#define I2C_SCL           18

// 업데이트 간격 (ms)
#define SENSOR_UPDATE_INTERVAL 1000
#define CAMERA_REFRESH_INTERVAL 5000

void initConfig();

#endif