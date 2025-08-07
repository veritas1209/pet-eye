// include/camera_manager.h - T-Camera S3 카메라 관리
#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>

// ESP32 카메라 라이브러리 include (조건부)
#ifdef ESP32
#include "esp_camera.h"
#endif

// T-Camera S3 핀 정의
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

// 카메라 설정
struct CameraConfig {
    int frame_size;                    // framesize_t 대신 int 사용
    int pixel_format;                  // pixformat_t 대신 int 사용
    int jpeg_quality;
    int fb_count;
    bool auto_capture;
    unsigned long capture_interval;
    unsigned long last_capture;
    bool camera_ready;
    int capture_count;
};

// 카메라 상태
extern CameraConfig camera_config;

// 카메라 관리 함수들
bool initCamera();
bool capturePhoto();

#ifdef ESP32
camera_fb_t* takePicture();
void releasePicture(camera_fb_t* fb);
void setCameraResolution(framesize_t size);
#endif

void setCameraQuality(int quality);
void handleCameraCapture();
void printCameraInfo();
bool checkCameraHealth();

#endif