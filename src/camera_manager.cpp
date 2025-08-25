#include "camera_manager.h"
#include <Wire.h>

// AXP2101 PMU 레지스터 주소
#define AXP2101_ADDR        0x34
#define AXP2101_ALDO1_VOLT  0x92
#define AXP2101_ALDO2_VOLT  0x93
#define AXP2101_ALDO4_VOLT  0x95
#define AXP2101_ALDO_ONOFF  0x90

// 간단한 AXP2101 PMU 초기화 (XPowersLib 없이)
bool initCameraPMU() {
    DebugSystem::log("Initializing AXP2101 PMU for camera power...");
    
    // I2C 초기화 (SDA=7, SCL=6)
    Wire.begin(7, 6);
    delay(10);
    
    // AXP2101 감지
    Wire.beginTransmission(AXP2101_ADDR);
    if (Wire.endTransmission() != 0) {
        DebugSystem::log("❌ AXP2101 PMU not found at 0x34");
        return false;
    }
    
    DebugSystem::log("✅ AXP2101 PMU detected");
    
    // ALDO1 = 1.8V (CAM DVDD)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO1_VOLT);
    Wire.write(0x0E);  // 1.8V setting
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("  ALDO1 set to 1.8V");
    }
    
    // ALDO2 = 2.8V (CAM DVDD)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO2_VOLT);
    Wire.write(0x1C);  // 2.8V setting
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("  ALDO2 set to 2.8V");
    }
    
    // ALDO4 = 3.0V (CAM AVDD)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO4_VOLT);
    Wire.write(0x1F);  // 3.0V setting
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("  ALDO4 set to 3.0V");
    }
    
    // ALDO1, ALDO2, ALDO4 전원 ON
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO_ONOFF);
    Wire.write(0xD6);  // Enable ALDO1, ALDO2, ALDO4
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("✅ Camera power rails enabled");
    }
    
    // 전원 안정화 대기
    delay(200);
    
    return true;
}

// I2C 장치 스캔
void scanI2CDevices() {
    DebugSystem::log("Scanning I2C bus for devices...");
    int deviceCount = 0;
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            String msg = "  I2C device found at 0x";
            if (address < 16) msg += "0";
            msg += String(address, HEX);
            
            // 알려진 장치 식별
            if (address == 0x34) msg += " (AXP2101 PMU)";
            else if (address == 0x3C || address == 0x30) msg += " (Camera sensor)";
            else if (address == 0x68) msg += " (MPU6050)";
            
            DebugSystem::log(msg);
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        DebugSystem::log("  No I2C devices found");
    } else {
        DebugSystem::log("  Total devices found: " + String(deviceCount));
    }
}

bool CameraManager::init() {
    if (!ENABLE_CAMERA) {
        DebugSystem::log("Camera disabled in config");
        return false;
    }
    
    DebugSystem::log("========== Camera Initialization Debug ==========");
    
    // Step 1: PMU 초기화 (필수)
    if (!initCameraPMU()) {
        DebugSystem::log("⚠️ WARNING: PMU initialization failed");
        DebugSystem::log("Camera may not work without proper power supply");
        // 그래도 계속 진행
    }
    
    // Step 2: 카메라 I2C 버스 스캔
    Wire.begin(5, 4);  // Camera I2C: SIOD=5, SIOC=4
    scanI2CDevices();
    
    // Step 3: 카메라 리셋 핀 테스트
    DebugSystem::log("Testing camera RESET pin (GPIO39)...");
    pinMode(39, OUTPUT);
    digitalWrite(39, LOW);
    delay(10);
    digitalWrite(39, HIGH);
    delay(10);
    DebugSystem::log("  Reset sequence completed");
    
    // Step 4: XCLK 신호 확인
    DebugSystem::log("Setting up XCLK pin (GPIO38)...");
    pinMode(38, OUTPUT);
    
    // Step 5: 카메라 설정
    DebugSystem::log("Configuring camera parameters...");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // T-Camera S3 핀 매핑 (LILYGO 공식)
    config.pin_d0 = 14;   // Y2
    config.pin_d1 = 47;   // Y3  
    config.pin_d2 = 48;   // Y4
    config.pin_d3 = 21;   // Y5
    config.pin_d4 = 13;   // Y6
    config.pin_d5 = 11;   // Y7
    config.pin_d6 = 10;   // Y8
    config.pin_d7 = 9;    // Y9
    
    config.pin_xclk = 38;
    config.pin_pclk = 12;
    config.pin_vsync = 8;
    config.pin_href = 18;
    config.pin_sccb_sda = 5;
    config.pin_sccb_scl = 4;
    config.pin_pwdn = -1;
    config.pin_reset = 39;
    
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    
    // PSRAM 설정
    if(psramFound()){
        DebugSystem::log("PSRAM found - using PSRAM for frame buffer");
        config.frame_size = FRAMESIZE_QVGA;  // 320x240
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        DebugSystem::log("No PSRAM - using DRAM");
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    
    // Step 6: 카메라 초기화
    DebugSystem::log("Initializing ESP32 camera driver...");
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        DebugSystem::log("❌ Camera init failed with error 0x" + String(err, HEX));
        
        // 상세 에러 분석
        switch(err) {
            case ESP_ERR_NOT_FOUND:
                DebugSystem::log("  Error: Camera sensor not found");
                DebugSystem::log("  Check: Camera ribbon cable connection");
                DebugSystem::log("  Check: PMU power supply");
                break;
            case ESP_ERR_NOT_SUPPORTED:
                DebugSystem::log("  Error: Camera sensor not supported");
                break;
            case ESP_ERR_INVALID_STATE:
                DebugSystem::log("  Error: Camera already initialized");
                break;
            case ESP_ERR_NO_MEM:
                DebugSystem::log("  Error: Out of memory");
                break;
            default:
                DebugSystem::log("  Unknown error code");
        }
        
        return false;
    }
    
    DebugSystem::log("✅ Camera driver initialized");
    
    // Step 7: 센서 설정
    sensor_t * s = esp_camera_sensor_get();
    if (s == nullptr) {
        DebugSystem::log("❌ Failed to get camera sensor");
        return false;
    }
    
    // 센서 정보
    DebugSystem::log("Camera sensor info:");
    DebugSystem::log("  PID: 0x" + String(s->id.PID, HEX));
    DebugSystem::log("  VER: 0x" + String(s->id.VER, HEX));
    DebugSystem::log("  MIDL: 0x" + String(s->id.MIDL, HEX));
    DebugSystem::log("  MIDH: 0x" + String(s->id.MIDH, HEX));
    
    // 센서별 설정
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
        DebugSystem::log("  OV3660 sensor configured");
    } else if (s->id.PID == OV2640_PID) {
        DebugSystem::log("  OV2640 sensor detected");
    } else {
        DebugSystem::log("  Unknown sensor type");
    }
    
    // T-Camera S3 이미지 방향 설정
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    
    // 프레임 크기 설정
    if (s->set_framesize(s, FRAMESIZE_QVGA) == 0) {
        DebugSystem::log("  Frame size set to QVGA (320x240)");
    }
    
    // Step 8: 테스트 캡처
    delay(500);  // 센서 안정화
    
    DebugSystem::log("Performing test capture...");
    camera_fb_t* test_fb = esp_camera_fb_get();
    
    if (test_fb) {
        DebugSystem::log("✅ Test capture successful!");
        DebugSystem::log("  Size: " + String(test_fb->len) + " bytes");
        DebugSystem::log("  Width: " + String(test_fb->width));
        DebugSystem::log("  Height: " + String(test_fb->height));
        DebugSystem::log("  Format: " + String(test_fb->format == PIXFORMAT_JPEG ? "JPEG" : "Other"));
        esp_camera_fb_return(test_fb);
        sysStatus.cameraInitialized = true;
    } else {
        DebugSystem::log("❌ Test capture failed");
        
        // 재시도
        delay(1000);
        test_fb = esp_camera_fb_get();
        if (test_fb) {
            DebugSystem::log("✅ Second test capture successful");
            esp_camera_fb_return(test_fb);
            sysStatus.cameraInitialized = true;
        } else {
            DebugSystem::log("❌ Camera initialization failed");
            sysStatus.cameraInitialized = false;
            
            // 전원 상태 재확인
            DebugSystem::log("Checking PMU status...");
            Wire.beginTransmission(AXP2101_ADDR);
            if (Wire.endTransmission() == 0) {
                DebugSystem::log("  PMU still responding");
            } else {
                DebugSystem::log("  PMU not responding - power issue likely");
            }
            
            return false;
        }
    }
    
    DebugSystem::log("========== Camera Init Complete ==========");
    return true;
}

camera_fb_t* CameraManager::capture() {
    if (!sysStatus.cameraInitialized) {
        DebugSystem::log("Camera not initialized");
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    
    if (!fb) {
        DebugSystem::log("Frame capture failed");
        
        // 상태 체크
        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            DebugSystem::log("Sensor exists but capture failed");
        } else {
            DebugSystem::log("Sensor lost!");
            sysStatus.cameraInitialized = false;
        }
        
        // PMU 상태 체크
        Wire.beginTransmission(AXP2101_ADDR);
        if (Wire.endTransmission() != 0) {
            DebugSystem::log("PMU not responding - power lost?");
        }
        
        // 재시도
        delay(100);
        fb = esp_camera_fb_get();
        if (!fb) {
            DebugSystem::log("Retry also failed");
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
    
    DebugSystem::log("=== Camera Test ===");
    
    // PMU 상태 체크
    DebugSystem::log("Checking PMU...");
    Wire.beginTransmission(AXP2101_ADDR);
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("  PMU OK");
    } else {
        DebugSystem::log("  PMU not responding");
    }
    
    // I2C 스캔
    scanI2CDevices();
    
    // 캡처 테스트
    DebugSystem::log("Attempting capture...");
    camera_fb_t *fb = capture();
    if (fb) {
        DebugSystem::log("✅ Capture successful");
        DebugSystem::log("  Size: " + String(fb->len) + " bytes");
        DebugSystem::log("  Resolution: " + String(fb->width) + "x" + String(fb->height));
        releaseFrame(fb);
        return true;
    } else {
        DebugSystem::log("❌ Capture failed");
        return false;
    }
}