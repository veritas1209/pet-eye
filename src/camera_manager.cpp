#include "camera_manager.h"
#include <Wire.h>
#include "driver/gpio.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

XPowersPMU PMU;

// PMU 초기화
bool initCameraPMU() {
    DebugSystem::log("Initializing AXP2101 PMU for Camera");
    
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) {
        DebugSystem::log("Failed to initialize PMU");
        return false;
    }
    
    DebugSystem::log("PMU initialized successfully");
    
    // 카메라 전원 설정 (공식 예제와 동일)
    PMU.setALDO1Voltage(1800);  // CAM DVDD 1.8V
    PMU.enableALDO1();
    
    PMU.setALDO2Voltage(2800);  // CAM AVDD 2.8V
    PMU.enableALDO2();
    
    PMU.setALDO4Voltage(3000);  // CAM DOVDD 3.0V
    PMU.enableALDO4();
    
    // TS Pin 비활성화 (충전 기능 사용시 필요)
    PMU.disableTSPinMeasure();
    
    DebugSystem::log("Camera power rails configured");
    delay(500);  // 전원 안정화
    
    return true;
}

bool CameraManager::init() {
    if (!ENABLE_CAMERA) {
        DebugSystem::log("Camera disabled in config");
        return false;
    }
    
    DebugSystem::log("========== Camera Initialization ==========");
    
    // Step 1: GPIO13 설정 (JTAG 해제)
    gpio_config_t conf = {};
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_ENABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.pin_bit_mask = 1LL << 13;
    gpio_config(&conf);
    DebugSystem::log("GPIO13 configured");
    
    // Step 2: PMU 초기화
    if (!initCameraPMU()) {
        DebugSystem::log("PMU initialization failed");
        return false;
    }
    
    // Step 3: 카메라 설정
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // 핀 매핑
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    
    // PSRAM 설정
    if(psramFound()){
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_QVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    
    // Step 4: 카메라 초기화
    DebugSystem::log("Initializing camera driver...");
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        DebugSystem::log("Camera init failed with error 0x" + String(err, HEX));
        return false;
    }
    
    DebugSystem::log("Camera driver initialized");
    
    // Step 5: 센서 설정
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);
            s->set_brightness(s, 1);
            s->set_saturation(s, -2);
        }
        
        // T-Camera S3 방향 설정
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
        s->set_framesize(s, FRAMESIZE_QVGA);
    }
    
    // Step 6: 테스트 캡처
    delay(500);
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
        DebugSystem::log("Test capture successful");
        esp_camera_fb_return(fb);
        sysStatus.cameraInitialized = true;
    } else {
        DebugSystem::log("Test capture failed");
        sysStatus.cameraInitialized = false;
    }
    
    DebugSystem::log("========== Camera Init Complete ==========");
    return sysStatus.cameraInitialized;
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
        return false;
    }
    
    camera_fb_t *fb = capture();
    if (fb) {
        DebugSystem::log("Capture test OK");
        releaseFrame(fb);
        return true;
    }
    return false;
}