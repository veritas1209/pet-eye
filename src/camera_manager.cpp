#include "camera_manager.h"
#include <Wire.h>

// AXP2101 PMU 레지스터 주소
#define AXP2101_ADDR        0x34
#define AXP2101_ALDO1_VOLT  0x92
#define AXP2101_ALDO2_VOLT  0x93
#define AXP2101_ALDO4_VOLT  0x95
#define AXP2101_ALDO_ONOFF  0x90
#define AXP2101_ALDO_STATUS 0x91

// TS Pin 비활성화 관련
#define AXP2101_CHG_V_CFG   0x64
#define AXP2101_ADC_CHANNEL_CTRL 0x30

// 전압 레벨 체크 함수
void checkPinVoltage(int pin, const char* pinName) {
    pinMode(pin, INPUT);
    delay(10);
    int digitalLevel = digitalRead(pin);
    
    if (pin <= 20) {
        int analogValue = analogRead(pin);
        float voltage = (analogValue / 4095.0) * 3.3;
        DebugSystem::log(String(pinName) + " (GPIO" + String(pin) + "): " + 
                        String(digitalLevel ? "HIGH" : "LOW") + 
                        ", Analog: " + String(analogValue) + 
                        ", Voltage: " + String(voltage, 2) + "V");
    } else {
        DebugSystem::log(String(pinName) + " (GPIO" + String(pin) + "): " + 
                        String(digitalLevel ? "HIGH" : "LOW"));
    }
}

// PMU 전원 상태 체크
bool checkPMUPowerStatus() {
    DebugSystem::log("=== PMU Power Status Check ===");
    
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(0x90);
    Wire.endTransmission();
    
    Wire.requestFrom(AXP2101_ADDR, (uint8_t)1);
    if (Wire.available()) {
        uint8_t status = Wire.read();
        DebugSystem::log("ALDO Status Register: 0x" + String(status, HEX));
        DebugSystem::log("  ALDO1 (1.8V): " + String((status & 0x01) ? "ON" : "OFF"));
        DebugSystem::log("  ALDO2 (2.8V): " + String((status & 0x02) ? "ON" : "OFF"));
        DebugSystem::log("  ALDO3: " + String((status & 0x04) ? "ON" : "OFF"));
        DebugSystem::log("  ALDO4 (3.0V): " + String((status & 0x08) ? "ON" : "OFF"));
        return (status & 0x0B) == 0x0B;
    }
    return false;
}

// 카메라 I2C 통신 테스트
bool testCameraI2C() {
    DebugSystem::log("=== Camera I2C Communication Test ===");
    
    uint8_t camera_addresses[] = {0x30, 0x3C, 0x3D, 0x21};
    bool found = false;
    
    for (int i = 0; i < 4; i++) {
        Wire.beginTransmission(camera_addresses[i]);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            DebugSystem::log("✅ Camera found at 0x" + String(camera_addresses[i], HEX));
            
            Wire.beginTransmission(camera_addresses[i]);
            Wire.write(0x0A);
            Wire.endTransmission();
            Wire.requestFrom(camera_addresses[i], (uint8_t)1);
            if (Wire.available()) {
                uint8_t pid = Wire.read();
                DebugSystem::log("  Sensor PID: 0x" + String(pid, HEX));
            }
            found = true;
        }
    }
    
    if (!found) {
        DebugSystem::log("❌ No camera sensor detected on I2C bus");
    }
    
    return found;
}

// PMU 초기화
bool initCameraPMU() {
    DebugSystem::log("=== AXP2101 PMU Initialization for Camera ===");
    
    Wire.begin(PMU_SDA, PMU_SCL);
    delay(100);
    
    // PMU 감지
    Wire.beginTransmission(AXP2101_ADDR);
    if (Wire.endTransmission() != 0) {
        DebugSystem::log("❌ AXP2101 PMU not found");
        return false;
    }
    
    DebugSystem::log("✅ AXP2101 PMU detected");
    
    // TS Pin 감지 비활성화 (중요!)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ADC_CHANNEL_CTRL);
    Wire.write(0x00);  // TS ADC 비활성화
    Wire.endTransmission();
    delay(10);
    
    // 현재 LDO 상태 읽기
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO_ONOFF);
    Wire.endTransmission();
    Wire.requestFrom(AXP2101_ADDR, (uint8_t)1);
    uint8_t current_ldo_status = 0;
    if (Wire.available()) {
        current_ldo_status = Wire.read();
        DebugSystem::log("Current LDO status: 0x" + String(current_ldo_status, HEX));
    }
    
    // ALDO1 전압 설정 (1.8V for camera DVDD)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO1_VOLT);
    Wire.write(0x1C);  // 1.8V (값 조정)
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("ALDO1 voltage set to 1.8V");
    }
    delay(10);
    
    // ALDO2 전압 설정 (2.8V for camera AVDD)  
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO2_VOLT);
    Wire.write(0x1C);  // 2.8V
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("ALDO2 voltage set to 2.8V");
    }
    delay(10);
    
    // ALDO4 전압 설정 (3.0V for camera IOVDD)
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO4_VOLT);
    Wire.write(0x1E);  // 3.0V
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("ALDO4 voltage set to 3.0V");
    }
    delay(10);
    
    // ALDO1, ALDO2, ALDO4 활성화
    // Bit 0: ALDO1, Bit 1: ALDO2, Bit 3: ALDO4
    uint8_t new_ldo_status = current_ldo_status | 0x0B;  // 0b00001011
    
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO_ONOFF);
    Wire.write(new_ldo_status);
    if (Wire.endTransmission() == 0) {
        DebugSystem::log("LDO enable register written: 0x" + String(new_ldo_status, HEX));
    }
    
    // 전원 안정화 대기 (매우 중요!)
    DebugSystem::log("Waiting 2 seconds for power stabilization...");
    delay(2000);
    
    // 최종 상태 확인
    Wire.beginTransmission(AXP2101_ADDR);
    Wire.write(AXP2101_ALDO_ONOFF);
    Wire.endTransmission();
    Wire.requestFrom(AXP2101_ADDR, (uint8_t)1);
    
    if (Wire.available()) {
        uint8_t final_status = Wire.read();
        DebugSystem::log("Final LDO status: 0x" + String(final_status, HEX));
        
        bool aldo1_on = (final_status & 0x01) != 0;
        bool aldo2_on = (final_status & 0x02) != 0;
        bool aldo4_on = (final_status & 0x08) != 0;
        
        DebugSystem::log("ALDO1 (1.8V): " + String(aldo1_on ? "ON ✅" : "OFF ❌"));
        DebugSystem::log("ALDO2 (2.8V): " + String(aldo2_on ? "ON ✅" : "OFF ❌"));
        DebugSystem::log("ALDO4 (3.0V): " + String(aldo4_on ? "ON ✅" : "OFF ❌"));
        
        if (aldo1_on && aldo2_on && aldo4_on) {
            DebugSystem::log("✅ Camera power successfully enabled!");
            return true;
        } else {
            DebugSystem::log("❌ Failed to enable all camera power rails");
            
            // 전압 값 다시 읽어보기
            for (uint8_t reg = 0x92; reg <= 0x95; reg++) {
                Wire.beginTransmission(AXP2101_ADDR);
                Wire.write(reg);
                Wire.endTransmission();
                Wire.requestFrom(AXP2101_ADDR, (uint8_t)1);
                if (Wire.available()) {
                    uint8_t val = Wire.read();
                    DebugSystem::log("Register 0x" + String(reg, HEX) + " = 0x" + String(val, HEX));
                }
            }
            return false;
        }
    }
    
    return false;
}

// 카메라 초기화
bool CameraManager::init() {
    if (!ENABLE_CAMERA) {
        DebugSystem::log("Camera disabled in config");
        return false;
    }
    
    DebugSystem::log("\n========== CAMERA HARDWARE DIAGNOSTICS ==========");
    
    // Step 1: 핀 전압 체크
    DebugSystem::log("\n=== Step 1: Pin Voltage Check ===");
    checkPinVoltage(Y2_GPIO_NUM, "Y2/D0");
    checkPinVoltage(Y3_GPIO_NUM, "Y3/D1");
    checkPinVoltage(Y4_GPIO_NUM, "Y4/D2");
    checkPinVoltage(Y5_GPIO_NUM, "Y5/D3");
    checkPinVoltage(Y6_GPIO_NUM, "Y6/D4");
    checkPinVoltage(Y7_GPIO_NUM, "Y7/D5");
    checkPinVoltage(Y8_GPIO_NUM, "Y8/D6");
    checkPinVoltage(Y9_GPIO_NUM, "Y9/D7");
    checkPinVoltage(XCLK_GPIO_NUM, "XCLK");
    checkPinVoltage(PCLK_GPIO_NUM, "PCLK");
    checkPinVoltage(VSYNC_GPIO_NUM, "VSYNC");
    checkPinVoltage(HREF_GPIO_NUM, "HREF");
    checkPinVoltage(SIOD_GPIO_NUM, "SIOD");
    checkPinVoltage(SIOC_GPIO_NUM, "SIOC");
    checkPinVoltage(RESET_GPIO_NUM, "RESET");
    
    // Step 2: PMU 초기화
    DebugSystem::log("\n=== Step 2: PMU Power Supply ===");
    if (!initCameraPMU()) {
        DebugSystem::log("❌ CRITICAL: PMU initialization failed!");
    }
    
    // Step 3: 카메라 리셋
    DebugSystem::log("\n=== Step 3: Camera Reset Sequence ===");
    pinMode(RESET_GPIO_NUM, OUTPUT);
    digitalWrite(RESET_GPIO_NUM, LOW);
    DebugSystem::log("RESET pin set LOW");
    delay(50);
    digitalWrite(RESET_GPIO_NUM, HIGH);
    DebugSystem::log("RESET pin set HIGH");
    delay(100);
    
    // Step 4: XCLK 신호
    DebugSystem::log("\n=== Step 4: XCLK Signal Generation ===");
    pinMode(XCLK_GPIO_NUM, OUTPUT);
    for (int i = 0; i < 10; i++) {
        digitalWrite(XCLK_GPIO_NUM, i % 2);
        delayMicroseconds(10);
    }
    
    // Step 5: I2C 초기화
    DebugSystem::log("\n=== Step 5: Camera I2C Bus Init ===");
    Wire.begin(SIOD_GPIO_NUM, SIOC_GPIO_NUM, 100000);
    delay(100);
    
    if (!testCameraI2C()) {
        DebugSystem::log("⚠️ Camera sensor not responding on I2C!");
    }
    
    // Step 6: ESP32 카메라 드라이버
    DebugSystem::log("\n=== Step 6: ESP32 Camera Driver Init ===");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    
    config.xclk_freq_hz = 5000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    if(psramFound()){
        DebugSystem::log("PSRAM detected");
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        DebugSystem::log("No PSRAM");
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    
    DebugSystem::log("Calling esp_camera_init()...");
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        DebugSystem::log("\n❌ CAMERA INIT FAILED!");
        DebugSystem::log("Error code: 0x" + String(err, HEX));
        
        checkPMUPowerStatus();
        testCameraI2C();
        
        return false;
    }
    
    DebugSystem::log("✅ Camera driver initialized!");
    
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        DebugSystem::log("Sensor PID: 0x" + String(s->id.PID, HEX));
        
        if (s->id.PID == OV3660_PID) {
            s->set_vflip(s, 1);
            s->set_brightness(s, 1);
            s->set_saturation(s, -2);
        }
        
        s->set_vflip(s, 1);
        s->set_hmirror(s, 1);
        s->set_framesize(s, FRAMESIZE_QVGA);
    }
    
    delay(500);
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
        DebugSystem::log("✅ Test capture successful!");
        esp_camera_fb_return(fb);
        sysStatus.cameraInitialized = true;
    } else {
        DebugSystem::log("❌ Test capture failed");
        sysStatus.cameraInitialized = false;
    }
    
    DebugSystem::log("========== END DIAGNOSTICS ==========\n");
    return sysStatus.cameraInitialized;
}

// 카메라 캡처
camera_fb_t* CameraManager::capture() {
    if (!sysStatus.cameraInitialized) {
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        DebugSystem::log("Frame capture failed");
        delay(100);
        fb = esp_camera_fb_get();
    }
    
    return fb;
}

// 프레임 해제
void CameraManager::releaseFrame(camera_fb_t* fb) {
    if (fb != nullptr) {
        esp_camera_fb_return(fb);
    }
}

// 초기화 상태 확인
bool CameraManager::isInitialized() {
    return sysStatus.cameraInitialized;
}

// 테스트 캡처
bool CameraManager::testCapture() {
    if (!sysStatus.cameraInitialized) {
        DebugSystem::log("Camera not initialized");
        return false;
    }
    
    DebugSystem::log("=== Camera Test ===");
    
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