#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H

#include "common.h"

// ESP32 카메라 관련 헤더는 조건부 포함
#ifdef ESP32
    #include "esp_camera.h"
#endif

class CameraModule {
private:
    bool initialized;
    #ifdef ESP32
    camera_config_t config;
    #endif

public:
    CameraModule();
    bool init();
    #ifdef ESP32
    camera_fb_t* captureImage();
    void releaseFrameBuffer(camera_fb_t* fb);
    #endif
    bool isInitialized() const { return initialized; }
    void adjustSettings();
};

extern CameraModule camera;

#endif