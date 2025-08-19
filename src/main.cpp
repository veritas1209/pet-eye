// 수정된 T-Camera S3 I2C 핀 설정 및 다중 핀맵 테스트
#include <Arduino.h>
#include "esp_camera.h"
#include <Wire.h>

// 가능한 I2C 핀 조합들 (T-Camera S3에서 시도해볼 수 있는 조합들)
struct I2C_Config {
    int sda_pin;
    int scl_pin;
    const char* name;
};

I2C_Config i2c_configs[] = {
    {8, 9, "ESP32-S3 기본 (SDA=8, SCL=9)"},
    {18, 19, "대안 1 (SDA=18, SCL=19)"},
    {21, 22, "ESP32 호환 (SDA=21, SCL=22)"},
    {4, 5, "대안 2 (SDA=4, SCL=5)"},
    {16, 17, "대안 3 (SDA=16, SCL=17)"},
    {6, 7, "대안 4 (SDA=6, SCL=7)"}
};

// T-Camera S3 카메라 핀 설정 (데이터 핀들은 고정)
struct CameraConfig {
    int xclk, sda, scl, vsync, href, pclk;
    const char* name;
};

CameraConfig camera_configs[] = {
    // 기본 I2C 핀 사용
    {4, 8, 9, 5, 27, 25, "기본 ESP32-S3 I2C"},
    {4, 18, 19, 5, 27, 25, "대안 I2C 1"},
    {15, 8, 9, 6, 7, 13, "대안 설정 1"},
    {15, 18, 19, 6, 7, 13, "대안 설정 2"}
};

bool test_i2c_connection(int sda, int scl) {
    Serial.printf("🔍 I2C 테스트: SDA=%d, SCL=%d... ", sda, scl);
    
    // Wire 라이브러리 재초기화
    Wire.end();
    delay(100);
    
    // 새로운 핀으로 I2C 시작
    bool success = Wire.begin(sda, scl);
    if (!success) {
        Serial.println("❌ 초기화 실패");
        return false;
    }
    
    Wire.setClock(100000);  // 100kHz로 설정
    delay(100);
    
    // I2C 스캔
    int device_count = 0;
    bool found_camera = false;
    
    for (byte addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            device_count++;
            Serial.printf("(0x%02X발견) ", addr);
            
            // 카메라 센서 주소 확인
            if (addr == 0x30 || addr == 0x21 || addr == 0x3C) {
                found_camera = true;
            }
        }
        delay(1);
    }
    
    if (device_count > 0) {
        Serial.printf("✅ %d개 디바이스", device_count);
        if (found_camera) {
            Serial.println(" (카메라 가능성!)");
            return true;
        } else {
            Serial.println(" (카메라 아님)");
        }
    } else {
        Serial.println("❌ 디바이스 없음");
    }
    
    return false;
}

bool find_working_i2c_pins() {
    Serial.println("🔍 작동하는 I2C 핀 조합 찾기...");
    
    int config_count = sizeof(i2c_configs) / sizeof(i2c_configs[0]);
    
    for (int i = 0; i < config_count; i++) {
        Serial.printf("\n📌 %s\n", i2c_configs[i].name);
        
        if (test_i2c_connection(i2c_configs[i].sda_pin, i2c_configs[i].scl_pin)) {
            Serial.printf("🎉 성공! SDA=%d, SCL=%d에서 카메라 감지됨\n", 
                         i2c_configs[i].sda_pin, i2c_configs[i].scl_pin);
            return true;
        }
    }
    
    Serial.println("\n💥 모든 I2C 핀 조합에서 카메라 감지 실패");
    return false;
}

bool test_camera_with_config(const CameraConfig& config) {
    Serial.printf("\n🔧 카메라 설정 테스트: %s\n", config.name);
    Serial.printf("   XCLK=%d, SDA=%d, SCL=%d, VSYNC=%d, HREF=%d, PCLK=%d\n",
                  config.xclk, config.sda, config.scl, config.vsync, config.href, config.pclk);
    
    // 기존 카메라 해제
    esp_camera_deinit();
    delay(200);
    
    // I2C 재설정
    Wire.end();
    Wire.begin(config.sda, config.scl);
    Wire.setClock(100000);
    delay(100);
    
    camera_config_t cam_config;
    memset(&cam_config, 0, sizeof(cam_config));
    
    // 안전한 카메라 설정
    cam_config.ledc_channel = LEDC_CHANNEL_0;
    cam_config.ledc_timer = LEDC_TIMER_0;
    
    // 데이터 핀들 (고정)
    cam_config.pin_d0 = 34;
    cam_config.pin_d1 = 13;
    cam_config.pin_d2 = 26;
    cam_config.pin_d3 = 35;
    cam_config.pin_d4 = 39;
    cam_config.pin_d5 = 38;
    cam_config.pin_d6 = 37;
    cam_config.pin_d7 = 36;
    
    // 제어 핀들 (테스트용)
    cam_config.pin_xclk = config.xclk;
    cam_config.pin_pclk = config.pclk;
    cam_config.pin_vsync = config.vsync;
    cam_config.pin_href = config.href;
    cam_config.pin_sccb_sda = config.sda;
    cam_config.pin_sccb_scl = config.scl;
    cam_config.pin_pwdn = -1;
    cam_config.pin_reset = -1;
    
    // 매우 안전한 설정
    cam_config.xclk_freq_hz = 8000000;  // 8MHz
    cam_config.pixel_format = PIXFORMAT_JPEG;
    cam_config.frame_size = FRAMESIZE_96X96;  // 최소 크기
    cam_config.jpeg_quality = 50;
    cam_config.fb_count = 1;
    cam_config.fb_location = CAMERA_FB_IN_DRAM;
    cam_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    // 카메라 초기화 시도
    Serial.println("   📸 초기화 중...");
    esp_err_t err = esp_camera_init(&cam_config);
    
    if (err != ESP_OK) {
        Serial.printf("   ❌ 실패: 0x%x (%s)\n", err, esp_err_to_name(err));
        return false;
    }
    
    Serial.println("   ✅ 초기화 성공!");
    
    // 프레임 테스트
    int success_count = 0;
    for (int i = 0; i < 3; i++) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb && fb->len > 10) {
            success_count++;
            Serial.printf("   📸 프레임 %d: %d bytes (%dx%d)\n", 
                         i+1, fb->len, fb->width, fb->height);
            esp_camera_fb_return(fb);
        } else {
            Serial.printf("   ❌ 프레임 %d 실패\n", i+1);
            if (fb) esp_camera_fb_return(fb);
        }
        delay(500);
    }
    
    if (success_count >= 2) {
        Serial.printf("   🎉 성공! %d/3 프레임 캡처됨\n", success_count);
        return true;
    } else {
        Serial.printf("   ⚠️ 불안정: %d/3 프레임만 성공\n", success_count);
        return false;
    }
}

bool find_working_camera_config() {
    Serial.println("\n🚀 작동하는 카메라 설정 찾기...");
    
    int config_count = sizeof(camera_configs) / sizeof(camera_configs[0]);
    
    for (int i = 0; i < config_count; i++) {
        if (test_camera_with_config(camera_configs[i])) {
            Serial.printf("\n🎉 성공한 설정: %s\n", camera_configs[i].name);
            Serial.println("📋 사용할 핀 정의:");
            Serial.printf("   #define XCLK_GPIO_NUM     %d\n", camera_configs[i].xclk);
            Serial.printf("   #define SIOD_GPIO_NUM     %d\n", camera_configs[i].sda);
            Serial.printf("   #define SIOC_GPIO_NUM     %d\n", camera_configs[i].scl);
            Serial.printf("   #define VSYNC_GPIO_NUM    %d\n", camera_configs[i].vsync);
            Serial.printf("   #define HREF_GPIO_NUM     %d\n", camera_configs[i].href);
            Serial.printf("   #define PCLK_GPIO_NUM     %d\n", camera_configs[i].pclk);
            return true;
        }
    }
    
    Serial.println("\n💥 모든 카메라 설정 실패!");
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    
    Serial.println("🐾 T-Camera S3 I2C 및 카메라 핀 자동 감지");
    Serial.println("===========================================");
    Serial.printf("칩: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("PSRAM: %s (%d KB)\n", psramFound() ? "있음" : "없음", 
                  psramFound() ? ESP.getPsramSize()/1024 : 0);
    
    // 1단계: I2C 핀 찾기
    if (find_working_i2c_pins()) {
        Serial.println("\n" + String("=").substring(0, 40));
        
        // 2단계: 카메라 설정 찾기
        if (find_working_camera_config()) {
            Serial.println("\n🎉 T-Camera S3 설정 완료!");
            Serial.println("이제 라이브 스트리밍 모드로 전환합니다...");
        } else {
            Serial.println("\n⚠️ I2C는 작동하지만 카메라 초기화 실패");
        }
    } else {
        Serial.println("\n💥 I2C 연결 자체가 불가능합니다");
        Serial.println("🔧 하드웨어 점검 사항:");
        Serial.println("   1. 카메라 모듈 연결 상태 확인");
        Serial.println("   2. 케이블 재연결");
        Serial.println("   3. 전원 공급 확인");
        Serial.println("   4. 다른 T-Camera S3 보드로 테스트");
    }
}

void loop() {
    static unsigned long last_test = 0;
    static int frame_count = 0;
    
    if (millis() - last_test >= 5000) {  // 5초마다
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb && fb->len > 10) {
            frame_count++;
            Serial.printf("[%lu] 📸 라이브 #%d: %d bytes (%dx%d)\n", 
                         millis()/1000, frame_count, fb->len, fb->width, fb->height);
            esp_camera_fb_return(fb);
        } else {
            Serial.printf("[%lu] ❌ 라이브 프레임 실패\n", millis()/1000);
            if (fb) esp_camera_fb_return(fb);
        }
        last_test = millis();
    }
    
    delay(200);
}