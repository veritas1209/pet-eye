// src/camera_diagnostic.cpp - T-Camera S3 카메라 완전 진단
#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

// T-Camera S3 실제 핀 정의 (수정된 버전)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     40
#define SIOD_GPIO_NUM     17  // SDA
#define SIOC_GPIO_NUM     18  // SCL

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

bool diagnostic_camera_init() {
    Serial.println("🔍 T-Camera S3 카메라 완전 진단 시작...");
    
    // 1. 핀 상태 확인
    Serial.println("📌 핀 상태 진단:");
    Serial.printf("XCLK (GPIO%d): %s\n", XCLK_GPIO_NUM, digitalRead(XCLK_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("SIOD (GPIO%d): %s\n", SIOD_GPIO_NUM, digitalRead(SIOD_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("SIOC (GPIO%d): %s\n", SIOC_GPIO_NUM, digitalRead(SIOC_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("VSYNC (GPIO%d): %s\n", VSYNC_GPIO_NUM, digitalRead(VSYNC_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("HREF (GPIO%d): %s\n", HREF_GPIO_NUM, digitalRead(HREF_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("PCLK (GPIO%d): %s\n", PCLK_GPIO_NUM, digitalRead(PCLK_GPIO_NUM) ? "HIGH" : "LOW");
    
    // 2. PSRAM 확인
    if (psramFound()) {
        Serial.printf("✅ PSRAM 발견: %d bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("❌ PSRAM 없음 - 저화질 모드 필요");
    }
    
    // 3. 카메라 설정 (최소 설정으로 시작)
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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    // 매우 보수적인 설정으로 시작
    if (psramFound()) {
        Serial.println("🎥 PSRAM 모드: 고화질 설정");
        config.frame_size = FRAMESIZE_VGA;      // 640x480 (안전한 시작)
        config.jpeg_quality = 20;               // 낮은 화질 (빠른 처리)
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
        Serial.println("🎥 일반 모드: 저화질 설정");
        config.frame_size = FRAMESIZE_QVGA;     // 320x240
        config.jpeg_quality = 25;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    
    // 4. 카메라 초기화 시도
    Serial.println("📸 카메라 초기화 시도...");
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        Serial.printf("❌ 카메라 초기화 실패: 0x%x (%s)\n", err, esp_err_to_name(err));
        
        // 오류 코드별 진단
        switch (err) {
            case ESP_ERR_NOT_FOUND:
                Serial.println("🔍 진단: 카메라 모듈이 감지되지 않음");
                Serial.println("   - I2C 연결 (SIOD/SIOC) 확인");
                Serial.println("   - 카메라 모듈 전원 확인");
                break;
            case ESP_ERR_NOT_SUPPORTED:
                Serial.println("🔍 진단: 지원되지 않는 설정");
                Serial.println("   - 핀 설정 재확인 필요");
                break;
            case ESP_ERR_NO_MEM:
                Serial.println("🔍 진단: 메모리 부족");
                Serial.println("   - 프레임 버퍼 개수 줄이기");
                Serial.println("   - 해상도 낮추기");
                break;
            default:
                Serial.printf("🔍 진단: 알 수 없는 오류 (0x%x)\n", err);
        }
        return false;
    }
    
    Serial.println("✅ 카메라 초기화 성공!");
    
    // 5. 센서 정보 확인
    sensor_t* s = esp_camera_sensor_get();
    if (s != NULL) {
        Serial.printf("📷 센서 정보:\n");
        Serial.printf("   센서 ID: 0x%x\n", s->id.PID);
        
        // 센서 최적화 (스트리밍용)
        s->set_brightness(s, 0);     // 밝기 0
        s->set_contrast(s, 0);       // 대비 0  
        s->set_saturation(s, 0);     // 채도 0
        s->set_whitebal(s, 1);       // 화이트밸런스 자동
        s->set_awb_gain(s, 1);       // AWB 게인
        s->set_exposure_ctrl(s, 1);  // 자동 노출
        s->set_aec2(s, 0);           // AEC2 비활성화 (빠른 처리)
        s->set_gain_ctrl(s, 1);      // 게인 제어
        s->set_agc_gain(s, 0);       // AGC 게인
        s->set_bpc(s, 0);            // BPC 비활성화 (빠른 처리)
        s->set_wpc(s, 1);            // WPC 활성화
        s->set_raw_gma(s, 1);        // RAW GMA
        s->set_lenc(s, 1);           // 렌즈 보정
        s->set_hmirror(s, 0);        // 미러 비활성화
        s->set_vflip(s, 0);          // 플립 비활성화
        s->set_dcw(s, 1);            // DCW 활성화
        s->set_colorbar(s, 0);       // 컬러바 비활성화
        
        Serial.println("✅ 센서 스트리밍 최적화 완료");
    } else {
        Serial.println("❌ 센서 정보를 가져올 수 없습니다");
        return false;
    }
    
    // 6. 첫 번째 프레임 테스트
    Serial.println("📸 첫 프레임 캡처 테스트...");
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 프레임 캡처 실패");
        return false;
    }
    
    Serial.printf("✅ 첫 프레임 성공!\n");
    Serial.printf("   크기: %d bytes\n", fb->len);
    Serial.printf("   해상도: %dx%d\n", fb->width, fb->height);
    Serial.printf("   포맷: %s\n", fb->format == PIXFORMAT_JPEG ? "JPEG" : "RAW");
    
    esp_camera_fb_return(fb);
    
    // 7. 연속 프레임 테스트 (스트리밍 시뮬레이션)
    Serial.println("🎬 연속 프레임 테스트 (10프레임)...");
    int success_count = 0;
    unsigned long start_time = millis();
    
    for (int i = 0; i < 10; i++) {
        camera_fb_t* test_fb = esp_camera_fb_get();
        if (test_fb) {
            success_count++;
            Serial.printf("프레임 %d: %d bytes\n", i+1, test_fb->len);
            esp_camera_fb_return(test_fb);
        } else {
            Serial.printf("프레임 %d: 실패\n", i+1);
        }
        delay(100); // 100ms 간격 (10 FPS 시뮬레이션)
    }
    
    unsigned long elapsed = millis() - start_time;
    float actual_fps = success_count * 1000.0 / elapsed;
    
    Serial.printf("📊 연속 테스트 결과:\n");
    Serial.printf("   성공: %d/10 프레임\n", success_count);
    Serial.printf("   실제 FPS: %.2f\n", actual_fps);
    
    if (success_count >= 8) {
        Serial.println("✅ 카메라 스트리밍 준비 완료!");
        return true;
    } else {
        Serial.println("⚠️ 카메라 불안정 - 설정 조정 필요");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("🐾 T-Camera S3 카메라 진단 시작");
    Serial.printf("칩: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("힙 메모리: %d KB\n", ESP.getFreeHeap() / 1024);
    
    if (diagnostic_camera_init()) {
        Serial.println("🎉 카메라 진단 성공! 실시간 스트리밍 가능");
    } else {
        Serial.println("💥 카메라 진단 실패! 하드웨어 점검 필요");
    }
}

void loop() {
    // 실시간 프레임 캡처 테스트
    static unsigned long last_frame = 0;
    static int frame_count = 0;
    
    if (millis() - last_frame >= 1000) { // 1초마다
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            frame_count++;
            Serial.printf("[%lu] 📸 라이브 프레임 #%d: %d bytes (%dx%d)\n", 
                         millis()/1000, frame_count, fb->len, fb->width, fb->height);
            esp_camera_fb_return(fb);
        } else {
            Serial.printf("[%lu] ❌ 프레임 캡처 실패\n", millis()/1000);
        }
        last_frame = millis();
    }
    
    delay(10);
}