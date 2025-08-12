// LilyGO T-Camera S3 핀 자동 스캔 및 테스트
#include <Arduino.h>
#include "esp_camera.h"
#include <Wire.h>

// LilyGO T-Camera S3 가능한 핀 조합들
struct CameraPinSet {
    const char* name;
    int xclk, siod, sioc, vsync, href, pclk, pwdn, reset;
    int y9, y8, y7, y6, y5, y4, y3, y2;
};

// LilyGO T-Camera S3 알려진 핀 설정들
CameraPinSet pinSets[] = {
    // 조합 1: 일반적인 LilyGO T-Camera S3
    {"LilyGO 기본", 15, 4, 5, 6, 7, 13, -1, -1, 16, 17, 18, 12, 10, 8, 9, 11},
    
    // 조합 2: 대안 1 (일부 T-Camera S3에서 사용)
    {"LilyGO 대안1", 40, 17, 18, 21, 38, 11, -1, -1, 39, 41, 42, 12, 3, 14, 47, 13},
    
    // 조합 3: 대안 2
    {"LilyGO 대안2", 4, 18, 23, 25, 26, 21, -1, -1, 35, 34, 39, 36, 19, 18, 3, 16},
    
    // 조합 4: 대안 3 (최신 버전)
    {"LilyGO 최신", 2, 26, 25, 22, 23, 21, -1, -1, 35, 34, 39, 36, 20, 19, 18, 5}
};

void scanI2CForCamera() {
    Serial.println("\n🔍 I2C 카메라 센서 스캔...");
    
    // 일반적인 I2C 주소들로 스캔
    int i2c_pins[][2] = {{4, 5}, {17, 18}, {21, 22}, {26, 25}};
    
    for (int i = 0; i < 4; i++) {
        int sda = i2c_pins[i][0];
        int scl = i2c_pins[i][1];
        
        Serial.printf("I2C 테스트: SDA=%d, SCL=%d\n", sda, scl);
        
        Wire.begin(sda, scl);
        Wire.setClock(100000); // 100kHz로 낮춤
        
        bool found = false;
        for (int addr = 0x20; addr <= 0x3D; addr++) { // 카메라 센서 주소 범위
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                Serial.printf("   ✅ I2C 디바이스 발견: 0x%02X", addr);
                
                // 일반적인 카메라 센서 주소들
                switch (addr) {
                    case 0x21: Serial.println(" (OV2640 가능성)"); break;
                    case 0x30: Serial.println(" (OV2640 가능성)"); break;
                    case 0x3C: Serial.println(" (OV3660 가능성)"); break;
                    case 0x3D: Serial.println(" (OV5640 가능성)"); break;
                    default: Serial.println(" (알 수 없는 센서)"); break;
                }
                found = true;
            }
        }
        
        if (!found) {
            Serial.printf("   ❌ 카메라 센서 없음\n");
        }
        
        Wire.end();
        delay(100);
    }
}

bool testCameraWithPins(CameraPinSet& pins) {
    Serial.printf("\n🧪 핀 세트 테스트: %s\n", pins.name);
    Serial.printf("   XCLK=%d, SIOD=%d, SIOC=%d\n", pins.xclk, pins.siod, pins.sioc);
    
    // 이전 카메라 해제
    esp_camera_deinit();
    delay(200);
    
    // 카메라 설정
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = pins.y2;
    config.pin_d1 = pins.y3;
    config.pin_d2 = pins.y4;
    config.pin_d3 = pins.y5;
    config.pin_d4 = pins.y6;
    config.pin_d5 = pins.y7;
    config.pin_d6 = pins.y8;
    config.pin_d7 = pins.y9;
    config.pin_xclk = pins.xclk;
    config.pin_pclk = pins.pclk;
    config.pin_vsync = pins.vsync;
    config.pin_href = pins.href;
    config.pin_sccb_sda = pins.siod;
    config.pin_sccb_scl = pins.sioc;
    config.pin_pwdn = pins.pwdn;
    config.pin_reset = pins.reset;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    // PSRAM 모드 (최소 설정)
    config.frame_size = FRAMESIZE_QVGA;     // 320x240 작게 시작
    config.jpeg_quality = 20;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    
    // 핀 상태 체크
    Serial.printf("   핀 상태: XCLK=%s, SIOD=%s, SIOC=%s\n",
                 digitalRead(pins.xclk) ? "HIGH" : "LOW",
                 digitalRead(pins.siod) ? "HIGH" : "LOW",
                 digitalRead(pins.sioc) ? "HIGH" : "LOW");
    
    // 카메라 초기화 시도
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        Serial.printf("   ❌ 실패: 0x%x (%s)\n", err, esp_err_to_name(err));
        return false;
    }
    
    Serial.println("   ✅ 초기화 성공!");
    
    // 센서 확인
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("   📷 센서 ID: 0x%04X\n", s->id.PID);
        
        // 센서 타입 확인
        switch (s->id.PID) {
            case 0x2642: Serial.println("   🎉 OV2640 센서 감지!"); break;
            case 0x3660: Serial.println("   🎉 OV3660 센서 감지!"); break;
            case 0x5640: Serial.println("   🎉 OV5640 센서 감지!"); break;
            default: Serial.printf("   ❓ 알 수 없는 센서: 0x%04X\n", s->id.PID); break;
        }
    }
    
    // 프레임 테스트
    Serial.println("   📸 프레임 테스트...");
    camera_fb_t* fb = esp_camera_fb_get();
    
    if (fb && fb->len > 0) {
        Serial.printf("   🎉 성공! %d bytes (%dx%d)\n", fb->len, fb->width, fb->height);
        esp_camera_fb_return(fb);
        
        // 추가 프레임 테스트 (3개)
        Serial.println("   🔄 추가 프레임 테스트...");
        int success = 0;
        for (int i = 0; i < 3; i++) {
            camera_fb_t* test_fb = esp_camera_fb_get();
            if (test_fb && test_fb->len > 0) {
                success++;
                esp_camera_fb_return(test_fb);
                Serial.printf("     프레임 %d: OK\n", i+1);
            } else {
                Serial.printf("     프레임 %d: 실패\n", i+1);
            }
            delay(100);
        }
        
        if (success >= 2) {
            Serial.printf("   🎊 안정성 테스트 통과! (%d/3)\n", success);
            return true;
        } else {
            Serial.printf("   ⚠️ 불안정 (%d/3)\n", success);
            return false;
        }
    } else {
        Serial.println("   ❌ 프레임 캡처 실패");
        return false;
    }
}

void checkHardware() {
    Serial.println("\n🔧 하드웨어 체크...");
    
    // 중요한 GPIO 핀들 상태 확인
    int important_pins[] = {2, 4, 5, 11, 13, 15, 17, 18, 21, 40};
    Serial.println("주요 GPIO 상태:");
    
    for (int pin : important_pins) {
        pinMode(pin, INPUT);
        delay(10);
        Serial.printf("   GPIO%d: %s\n", pin, digitalRead(pin) ? "HIGH" : "LOW");
    }
    
    // 전원 관련 체크
    Serial.println("\n⚡ 전원 상태:");
    Serial.printf("   힙 메모리: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("   PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
    Serial.printf("   CPU 주파수: %d MHz\n", ESP.getCpuFreqMHz());
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🐾 LilyGO T-Camera S3 정밀 진단");
    Serial.println("================================");
    
    // 하드웨어 기본 체크
    checkHardware();
    
    // I2C 센서 스캔
    scanI2CForCamera();
    
    // 핀 조합 테스트
    Serial.println("\n🎯 핀 조합 자동 테스트...");
    
    bool found_working = false;
    int total_sets = sizeof(pinSets) / sizeof(pinSets[0]);
    
    for (int i = 0; i < total_sets; i++) {
        if (testCameraWithPins(pinSets[i])) {
            Serial.printf("\n🎉🎉🎉 성공한 핀 설정: %s 🎉🎉🎉\n", pinSets[i].name);
            Serial.println("✅ 이 설정을 기록해두세요:");
            Serial.println("----------------------------------------");
            Serial.printf("#define XCLK_GPIO_NUM     %d\n", pinSets[i].xclk);
            Serial.printf("#define SIOD_GPIO_NUM     %d\n", pinSets[i].siod);
            Serial.printf("#define SIOC_GPIO_NUM     %d\n", pinSets[i].sioc);
            Serial.printf("#define VSYNC_GPIO_NUM    %d\n", pinSets[i].vsync);
            Serial.printf("#define HREF_GPIO_NUM     %d\n", pinSets[i].href);
            Serial.printf("#define PCLK_GPIO_NUM     %d\n", pinSets[i].pclk);
            Serial.printf("#define Y9_GPIO_NUM       %d\n", pinSets[i].y9);
            Serial.printf("#define Y8_GPIO_NUM       %d\n", pinSets[i].y8);
            Serial.printf("#define Y7_GPIO_NUM       %d\n", pinSets[i].y7);
            Serial.printf("#define Y6_GPIO_NUM       %d\n", pinSets[i].y6);
            Serial.printf("#define Y5_GPIO_NUM       %d\n", pinSets[i].y5);
            Serial.printf("#define Y4_GPIO_NUM       %d\n", pinSets[i].y4);
            Serial.printf("#define Y3_GPIO_NUM       %d\n", pinSets[i].y3);
            Serial.printf("#define Y2_GPIO_NUM       %d\n", pinSets[i].y2);
            Serial.println("----------------------------------------");
            found_working = true;
            break;
        }
        
        delay(500); // 테스트 간격
    }
    
    if (!found_working) {
        Serial.println("\n💥 모든 핀 조합 실패!");
        Serial.println("\n🔧 물리적 점검 필요사항:");
        Serial.println("1. 카메라 플렉스 케이블 완전히 분리 후 재연결");
        Serial.println("2. 케이블 방향 확인 (금속 접점이 아래로)");
        Serial.println("3. 커넥터 래치 완전히 잠김 확인");
        Serial.println("4. 보드 뒷면 'T-Camera S3' 표기 확인");
        Serial.println("5. 다른 USB 케이블로 테스트");
        Serial.println("6. 카메라 모듈 자체 불량 가능성");
        
        Serial.println("\n📞 LilyGO 지원:");
        Serial.println("GitHub: https://github.com/Xinyuan-LilyGO/T-Camera-S3");
    }
}

void loop() {
    delay(5000);
    Serial.printf("[%lu초] 메모리: %dKB | PSRAM: %dKB\n", 
                 millis()/1000, 
                 ESP.getFreeHeap()/1024, 
                 ESP.getFreePsram()/1024);
}