#include "camera_manager.h"
#include <Wire.h>

// AXP2101 PMU 지원 (옵션)
#ifdef USE_AXP2101_PMU
#include "XPowersLib.h"
XPowersPMU PMU;
#endif

bool CameraManager::init() {
    if (!ENABLE_CAMERA) {
        DebugSystem::log("Camera disabled in config");
        return false;
    }
    
    DebugSystem::log("Initializing camera...");
    
    // AXP2101 PMU 초기화 시도 (T-Camera S3에 있을 수 있음)
    #ifdef USE_AXP2101_PMU
    Wire.begin(7, 6);  // I2C_SDA=7, I2C_SCL=6
    if (PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, 7, 6)) {
        // 카메라 전원 설정
        PMU.setALDO1Voltage(1800);  // CAM DVDD 1.8V
        PMU.enableALDO1();
        PMU.setALDO2Voltage(2800);  // CAM DVDD 2.8V
        PMU.enableALDO2();
        PMU.setALDO4Voltage(3000);  // CAM AVDD 3.0V
        PMU.enableALDO4();
        
        // TS Pin detection must be disable
        PMU.disableTSPinMeasure();
        
        DebugSystem::log("✅ PMU initialized - Camera power enabled");
        delay(100);  // 전원 안정화 대기
    } else {
        DebugSystem::log("PMU not found - continuing without PMU");
    }
    #endif
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // T-Camera S3 핀 설정 (LILYGO 공식 예제 기준)
    config.pin_d0 = 14;   // Y2_GPIO_NUM
    config.pin_d1 = 47;   // Y3_GPIO_NUM
    config.pin_d2 = 48;   // Y4_GPIO_NUM
    config.pin_d3 = 21;   // Y5_GPIO_NUM
    config.pin_d4 = 13;   // Y6_GPIO_NUM
    config.pin_d5 = 11;   // Y7_GPIO_NUM
    config.pin_d6 = 10;   // Y8_GPIO_NUM
    config.pin_d7 = 9;    // Y9_GPIO_NUM
    
    config.pin_xclk = 38;     // XCLK_GPIO_NUM
    config.pin_pclk = 12;     // PCLK_GPIO_NUM
    config.pin_vsync = 8;     // VSYNC_GPIO_NUM
    config.pin_href = 18;     // HREF_GPIO_NUM
    config.pin_sccb_sda = 5;  // SIOD_GPIO_NUM
    config.pin_sccb_scl = 4;  // SIOC_GPIO_NUM
    config.pin_pwdn = -1;     // PWDN_GPIO_NUM (not used)
    config.pin_reset = 39;    // RESET_GPIO_NUM
    
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // PSRAM 설정
    if(psramFound()){
        DebugSystem::log("PSRAM found - using PSRAM for frame buffer");
        config.frame_size = FRAMESIZE_QVGA;  // 320x240 시작
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        DebugSystem::log("No PSRAM - using DRAM for frame buffer");
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    
    // 카메라 초기화
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        DebugSystem::log("❌ Camera init failed with error 0x" + String(err, HEX));
        
        // 에러 코드별 상세 메시지
        if (err == ESP_ERR_NOT_FOUND) {
            DebugSystem::log("Camera not found - check connections");
        } else if (err == ESP_ERR_NOT_SUPPORTED) {
            DebugSystem::log("Camera not supported");
        } else if (err == ESP_ERR_INVALID_STATE) {
            DebugSystem::log("Camera already initialized");
        }
        
        return false;
    }
    
    // 센서 설정
    sensor_t * s = esp_camera_sensor_get();
    if (s == nullptr) {
        DebugSystem::log("❌ Failed to get camera sensor");
        return false;
    }
    
    // 센서 정보 출력
    DebugSystem::log("Camera sensor PID: 0x" + String(s->id.PID, HEX));
    
    // OV3660 센서 설정 (T-Camera S3에서 주로 사용)
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);     // 수직 플립
        s->set_brightness(s, 1); // 밝기 조정
        s->set_saturation(s, -2); // 채도 조정
        DebugSystem::log("OV3660 sensor detected and configured");
    }
    // OV2640 센서 설정
    else if (s->id.PID == OV2640_PID) {
        DebugSystem::log("OV2640 sensor detected");
    }
    
    // T-Camera S3 특정 설정 (이미지 방향)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    
    // 프레임 크기를 QVGA로 설정
    if (s->set_framesize(s, FRAMESIZE_QVGA) != 0) {
        DebugSystem::log("⚠️ Failed to set framesize");
    }
    
    // 안정화 대기
    delay(500);
    
    // 테스트 캡처
    DebugSystem::log("Performing test capture...");
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (test_fb) {
        DebugSystem::log("✅ Test capture successful:");
        DebugSystem::log("  Size: " + String(test_fb->len) + " bytes");
        DebugSystem::log("  Resolution: " + String(test_fb->width) + "x" + String(test_fb->height));
        DebugSystem::log("  Format: " + String(test_fb->format));
        esp_camera_fb_return(test_fb);
        sysStatus.cameraInitialized = true;
    } else {
        DebugSystem::log("❌ Test capture failed - camera may not be ready");
        
        // 한 번 더 시도
        delay(1000);
        test_fb = esp_camera_fb_get();
        if (test_fb) {
            DebugSystem::log("✅ Second test capture successful");
            esp_camera_fb_return(test_fb);
            sysStatus.cameraInitialized = true;
        } else {
            DebugSystem::log("❌ Camera initialization failed after retry");
            sysStatus.cameraInitialized = false;
            return false;
        }
    }
    
    DebugSystem::log("✅ Camera initialized successfully");
    return true;
}

camera_fb_t* CameraManager::capture() {
    if (!sysStatus.cameraInitialized) {
        DebugSystem::log("Camera not initialized in capture()");
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    
    if (!fb) {
        DebugSystem::log("esp_camera_fb_get() returned NULL");
        
        // 카메라 센서 상태 확인
        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            DebugSystem::log("Camera sensor exists but fb_get failed");
            DebugSystem::log("Sensor status - AEC: " + String(s->status.aec) + 
                           ", AGC: " + String(s->status.agc));
        } else {
            DebugSystem::log("Camera sensor is NULL!");
            sysStatus.cameraInitialized = false;
        }
        
        // 재시도
        delay(100);
        fb = esp_camera_fb_get();
        if (!fb) {
            DebugSystem::log("Second attempt also failed");
            
            // 카메라 재초기화 시도 (옵션)
            static unsigned long lastReinitTime = 0;
            if (millis() - lastReinitTime > 30000) {  // 30초마다 재초기화 시도
                DebugSystem::log("Attempting camera reinit...");
                esp_camera_deinit();
                delay(100);
                if (init()) {
                    fb = esp_camera_fb_get();
                }
                lastReinitTime = millis();
            }
        }
    } else {
        // 성공 시 프레임 정보 로그 (주기적으로)
        static unsigned long lastLogTime = 0;
        if (millis() - lastLogTime > 10000) {  // 10초마다
            DebugSystem::log("Frame captured: " + String(fb->len) + " bytes, " +
                           String(fb->width) + "x" + String(fb->height));
            lastLogTime = millis();
        }
    }
    
    return fb;
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
    
    DebugSystem::log("Testing camera capture...");
    
    camera_fb_t *fb = capture();
    if (fb) {
        DebugSystem::log("✅ Camera test successful:");
        DebugSystem::log("  Frame size: " + String(fb->len) + " bytes");
        DebugSystem::log("  Resolution: " + String(fb->width) + "x" + String(fb->height));
        DebugSystem::log("  Format: " + String(fb->format == PIXFORMAT_JPEG ? "JPEG" : "Other"));
        releaseFrame(fb);
        return true;
    } else {
        DebugSystem::log("❌ Camera test failed - Could not capture frame");
        return false;
    }
}