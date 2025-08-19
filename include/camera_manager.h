// include/camera_manager.h - 수정된 카메라 헤더
#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>

// ESP32 카메라 라이브러리 include
#include "esp_camera.h"

// 카메라 설정 구조체
struct CameraConfig {
    int frame_size;
    int pixel_format;
    int jpeg_quality;
    int fb_count;
    bool auto_capture;
    unsigned long capture_interval;
    unsigned long last_capture;
    bool camera_ready;
    int capture_count;
};

// 전역 변수 선언 (extern으로 중복 방지)
extern CameraConfig camera_config;

// 카메라 관리 함수들
bool initCamera();
bool capturePhoto();
camera_fb_t* takePicture();
void releasePicture(camera_fb_t* fb);
void setCameraResolution(framesize_t size);
void setCameraQuality(int quality);
void handleCameraCapture();
void printCameraInfo();
bool checkCameraHealth();

#endif