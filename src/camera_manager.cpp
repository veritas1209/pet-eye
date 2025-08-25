#include "camera_manager.h"

bool CameraManager::init() {
    if (!ENABLE_CAMERA) {
        DebugSystem::log("Camera disabled in config");
        return false;
    }
    
    DebugSystem::log("Initializing camera...");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;  // ArduinoJson 7.x에서 수정된 이름
    config.pin_sccb_scl = CAM_PIN_SIOC;  // ArduinoJson 7.x에서 수정된 이름
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // PSRAM 설정
    if(psramFound()){
        config.frame_size = FRAMESIZE_VGA;  // 640x480
        config.jpeg_quality = 12;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    } else {
        config.frame_size = FRAMESIZE_QVGA;  // 320x240
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    
    // 카메라 초기화
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        DebugSystem::log("❌ Camera init failed: 0x" + String(err, HEX));
        return false;
    }
    
    sysStatus.cameraInitialized = true;
    DebugSystem::log("✅ Camera initialized successfully");
    return true;
}

camera_fb_t* CameraManager::capture() {
    if (!sysStatus.cameraInitialized) {
        return nullptr;
    }
    return esp_camera_fb_get();
}

void CameraManager::releaseFrame(camera_fb_t* fb) {
    if (fb != nullptr) {
        esp_camera_fb_return(fb);
    }
}

bool CameraManager::isInitialized() {
    return sysStatus.cameraInitialized;
}

bool CameraManager::testCapture() {
    if (!sysStatus.cameraInitialized) {
        DebugSystem::log("Camera not initialized");
        return false;
    }
    
    camera_fb_t *fb = capture();
    if (fb) {
        DebugSystem::log("Camera test successful - Frame size: " + String(fb->len) + " bytes");
        releaseFrame(fb);
        return true;
    } else {
        DebugSystem::log("Camera test failed - Could not capture frame");
        return false;
    }
}