// src/camera_manager.cpp - 기본 초기화부터 단계별 디버깅
#include "camera_manager.h"
#include "config.h"

// 카메라 설정 전역 변수 (명시적 초기화)
CameraConfig camera_config = {
    .frame_size = 0,
    .pixel_format = 0,
    .jpeg_quality = 12,
    .fb_count = 1,
    .auto_capture = false,
    .capture_interval = 30000,
    .last_capture = 0,
    .camera_ready = false,
    .capture_count = 0
};

// LilyGO T-Camera S3 핀 정의 (하드웨어 확인된 핀)
#define LILYGO_PWDN_GPIO_NUM     -1
#define LILYGO_RESET_GPIO_NUM    -1
#define LILYGO_XCLK_GPIO_NUM     15
#define LILYGO_SIOD_GPIO_NUM     4
#define LILYGO_SIOC_GPIO_NUM     5

#define LILYGO_Y9_GPIO_NUM       16
#define LILYGO_Y8_GPIO_NUM       17
#define LILYGO_Y7_GPIO_NUM       18
#define LILYGO_Y6_GPIO_NUM       12
#define LILYGO_Y5_GPIO_NUM       10
#define LILYGO_Y4_GPIO_NUM       8
#define LILYGO_Y3_GPIO_NUM       9
#define LILYGO_Y2_GPIO_NUM       11
#define LILYGO_VSYNC_GPIO_NUM    6
#define LILYGO_HREF_GPIO_NUM     7
#define LILYGO_PCLK_GPIO_NUM     13

void debugGPIOStates() {
    Serial.println("🔍 GPIO 상태 디버깅:");
    
    // 중요한 카메라 핀들 상태 확인
    Serial.printf("  XCLK (GPIO%d): %s\n", LILYGO_XCLK_GPIO_NUM, 
                 digitalRead(LILYGO_XCLK_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("  SIOD (GPIO%d): %s\n", LILYGO_SIOD_GPIO_NUM, 
                 digitalRead(LILYGO_SIOD_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("  SIOC (GPIO%d): %s\n", LILYGO_SIOC_GPIO_NUM, 
                 digitalRead(LILYGO_SIOC_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("  VSYNC (GPIO%d): %s\n", LILYGO_VSYNC_GPIO_NUM, 
                 digitalRead(LILYGO_VSYNC_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("  HREF (GPIO%d): %s\n", LILYGO_HREF_GPIO_NUM, 
                 digitalRead(LILYGO_HREF_GPIO_NUM) ? "HIGH" : "LOW");
    Serial.printf("  PCLK (GPIO%d): %s\n", LILYGO_PCLK_GPIO_NUM, 
                 digitalRead(LILYGO_PCLK_GPIO_NUM) ? "HIGH" : "LOW");
}

void debugSystemState() {
    Serial.println("🔍 시스템 상태 디버깅:");
    Serial.printf("  ESP32-S3 칩: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("  CPU 주파수: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  힙 메모리: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  PSRAM 상태: %s\n", psramFound() ? "발견됨" : "없음");
    if (psramFound()) {
        Serial.printf("  PSRAM 크기: %d bytes\n", ESP.getPsramSize());
        Serial.printf("  PSRAM 여유: %d bytes\n", ESP.getFreePsram());
    }
    Serial.printf("  플래시 크기: %d bytes\n", ESP.getFlashChipSize());
}

bool initCamera() {
    Serial.println("📷 === 카메라 기본 초기화 시작 ===");
    
    // 1. 시스템 상태 확인
    debugSystemState();
    
    // 2. GPIO 상태 확인
    debugGPIOStates();
    
    // 3. 카메라 설정 구조체 명시적 초기화
    Serial.println("📷 카메라 설정 구조체 초기화...");
    
    camera_config_t config;
    memset(&config, 0, sizeof(camera_config_t)); // 메모리 초기화
    
    // 기본 설정
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    // 데이터 핀 설정
    config.pin_d0 = LILYGO_Y2_GPIO_NUM;
    config.pin_d1 = LILYGO_Y3_GPIO_NUM;
    config.pin_d2 = LILYGO_Y4_GPIO_NUM;
    config.pin_d3 = LILYGO_Y5_GPIO_NUM;
    config.pin_d4 = LILYGO_Y6_GPIO_NUM;
    config.pin_d5 = LILYGO_Y7_GPIO_NUM;
    config.pin_d6 = LILYGO_Y8_GPIO_NUM;
    config.pin_d7 = LILYGO_Y9_GPIO_NUM;
    
    // 클럭 및 제어 핀 설정
    config.pin_xclk = LILYGO_XCLK_GPIO_NUM;
    config.pin_pclk = LILYGO_PCLK_GPIO_NUM;
    config.pin_vsync = LILYGO_VSYNC_GPIO_NUM;
    config.pin_href = LILYGO_HREF_GPIO_NUM;
    
    // I2C 핀 설정 (가장 중요!)
    config.pin_sccb_sda = LILYGO_SIOD_GPIO_NUM;
    config.pin_sccb_scl = LILYGO_SIOC_GPIO_NUM;
    
    // 전원 제어 핀
    config.pin_pwdn = LILYGO_PWDN_GPIO_NUM;
    config.pin_reset = LILYGO_RESET_GPIO_NUM;
    
    // 클럭 및 포맷 설정
    config.xclk_freq_hz = 20000000;  // 20MHz
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    Serial.println("📷 핀 설정 완료:");
    Serial.printf("  XCLK: GPIO%d\n", config.pin_xclk);
    Serial.printf("  SDA:  GPIO%d\n", config.pin_sccb_sda);
    Serial.printf("  SCL:  GPIO%d\n", config.pin_sccb_scl);
    Serial.printf("  클럭: %d Hz\n", config.xclk_freq_hz);
    
    // 4. 메모리 설정 (보수적으로 시작)
    Serial.println("📷 메모리 설정...");
    
    if (psramFound()) {
        Serial.println("  PSRAM 모드 - 고화질 설정");
        config.frame_size = FRAMESIZE_VGA;      // 640x480 (안정적 시작)
        config.jpeg_quality = 15;               // 중간 화질
        config.fb_count = 2;
        config.fb_location = CAMERA_FB_IN_PSRAM;
    } else {
        Serial.println("  DRAM 모드 - 저화질 설정");
        config.frame_size = FRAMESIZE_QVGA;     // 320x240
        config.jpeg_quality = 20;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
    
    Serial.printf("  프레임 크기: %d\n", config.frame_size);
    Serial.printf("  JPEG 품질: %d\n", config.jpeg_quality);
    Serial.printf("  버퍼 개수: %d\n", config.fb_count);
    
    // 5. 카메라 초기화 시도
    Serial.println("📷 ESP 카메라 라이브러리 초기화 시도...");
    
    esp_err_t err = esp_camera_init(&config);
    
    if (err != ESP_OK) {
        Serial.printf("❌ 카메라 초기화 실패!\n");
        Serial.printf("   오류 코드: 0x%x\n", err);
        Serial.printf("   오류 이름: %s\n", esp_err_to_name(err));
        
        // 상세 오류 분석
        switch (err) {
            case ESP_ERR_NOT_FOUND:
                Serial.println("   📋 분석: 카메라 센서를 찾을 수 없습니다");
                Serial.println("   🔧 체크: I2C 통신 (SDA/SCL 핀)");
                break;
            case ESP_ERR_NO_MEM:
                Serial.println("   📋 분석: 메모리 부족");
                Serial.println("   🔧 체크: 프레임 버퍼 크기");
                break;
            case ESP_ERR_NOT_SUPPORTED:
                Serial.println("   📋 분석: 지원되지 않는 설정");
                Serial.println("   🔧 체크: 핀 설정 및 보드 타입");
                break;
            case ESP_ERR_INVALID_ARG:
                Serial.println("   📋 분석: 잘못된 매개변수");
                Serial.println("   🔧 체크: config 구조체 설정");
                break;
            default:
                Serial.printf("   📋 분석: 알 수 없는 오류 (0x%x)\n", err);
        }
        
        // 변수 상태 초기화
        camera_config.camera_ready = false;
        camera_config.frame_size = 0;
        camera_config.pixel_format = 0;
        
        return false;
    }
    
    Serial.println("✅ 카메라 하드웨어 초기화 성공!");
    
    // 6. 센서 정보 확인
    Serial.println("📷 센서 정보 확인 중...");
    
    sensor_t* s = esp_camera_sensor_get();
    if (s == NULL) {
        Serial.println("❌ 센서 정보를 가져올 수 없습니다");
        camera_config.camera_ready = false;
        return false;
    }
    
    Serial.printf("✅ 센서 감지됨!\n");
    Serial.printf("   센서 ID: 0x%04X\n", s->id.PID);
    
    // 센서 타입 확인
    switch (s->id.PID) {
        case 0x2642:
            Serial.println("   센서 타입: OV2640 (LilyGO 표준)");
            break;
        case 0x3660:
            Serial.println("   센서 타입: OV3660 (LilyGO 고급)");
            break;
        case 0x5640:
            Serial.println("   센서 타입: OV5640 (LilyGO 최고급)");
            break;
        default:
            Serial.printf("   센서 타입: 알 수 없음 (0x%04X)\n", s->id.PID);
            break;
    }
    
    // 7. 기본 센서 설정
    Serial.println("📷 센서 기본 설정 적용 중...");
    
    s->set_brightness(s, 0);      // 밝기 0
    s->set_contrast(s, 0);        // 대비 0
    s->set_saturation(s, 0);      // 채도 0
    s->set_whitebal(s, 1);        // 자동 화이트밸런스
    s->set_awb_gain(s, 1);        // AWB 게인
    s->set_exposure_ctrl(s, 1);   // 자동 노출
    
    Serial.println("✅ 센서 기본 설정 완료");
    
    // 8. 내부 변수 상태 초기화
    Serial.println("📷 내부 변수 초기화 중...");
    
    camera_config.frame_size = config.frame_size;
    camera_config.pixel_format = config.pixel_format;
    camera_config.jpeg_quality = config.jpeg_quality;
    camera_config.fb_count = config.fb_count;
    camera_config.auto_capture = true;
    camera_config.capture_interval = CAMERA_CAPTURE_INTERVAL;
    camera_config.last_capture = 0;
    camera_config.capture_count = 0;
    camera_config.camera_ready = true;  // 최종 성공 상태
    
    Serial.println("✅ 내부 변수 초기화 완료");
    
    // 9. 첫 번째 프레임 테스트
    Serial.println("📷 첫 프레임 캡처 테스트...");
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == NULL) {
        Serial.println("❌ 첫 프레임 캡처 실패");
        camera_config.camera_ready = false;
        return false;
    }
    
    if (fb->len == 0) {
        Serial.println("❌ 빈 프레임 수신");
        esp_camera_fb_return(fb);
        camera_config.camera_ready = false;
        return false;
    }
    
    Serial.printf("🎉 첫 프레임 성공!\n");
    Serial.printf("   크기: %d bytes\n", fb->len);
    Serial.printf("   해상도: %dx%d\n", fb->width, fb->height);
    Serial.printf("   포맷: %s\n", fb->format == PIXFORMAT_JPEG ? "JPEG" : "RAW");
    
    esp_camera_fb_return(fb);
    
    // 10. 안정성 테스트 (3회 연속)
    Serial.println("📷 안정성 테스트 (3회 연속)...");
    
    int success_count = 0;
    for (int i = 0; i < 3; i++) {
        camera_fb_t* test_fb = esp_camera_fb_get();
        if (test_fb && test_fb->len > 0) {
            success_count++;
            Serial.printf("   테스트 %d: 성공 (%d bytes)\n", i+1, test_fb->len);
            esp_camera_fb_return(test_fb);
        } else {
            Serial.printf("   테스트 %d: 실패\n", i+1);
            if (test_fb) esp_camera_fb_return(test_fb);
        }
        delay(100);
    }
    
    if (success_count >= 2) {
        Serial.printf("🎉 안정성 테스트 통과! (%d/3)\n", success_count);
        Serial.println("📷 === 카메라 초기화 완전 성공! ===");
        return true;
    } else {
        Serial.printf("⚠️  안정성 테스트 부분 실패 (%d/3)\n", success_count);
        Serial.println("   카메라가 불안정할 수 있습니다");
        camera_config.camera_ready = false;
        return false;
    }
}

bool capturePhoto() {
    if (!camera_config.camera_ready) {
        Serial.println("❌ 카메라가 준비되지 않음 (capture)");
        return false;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 프레임 버퍼 획득 실패");
        return false;
    }
    
    if (fb->len == 0) {
        Serial.println("❌ 빈 프레임");
        esp_camera_fb_return(fb);
        return false;
    }
    
    Serial.printf("📸 촬영 성공 - %d bytes (%dx%d)\n", 
                  fb->len, fb->width, fb->height);
    
    esp_camera_fb_return(fb);
    camera_config.capture_count++;
    
    return true;
}

camera_fb_t* takePicture() {
    if (!camera_config.camera_ready) {
        Serial.println("❌ 카메라가 준비되지 않음 (take)");
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ takePicture 실패");
        return nullptr;
    }
    
    if (fb->len == 0) {
        Serial.println("❌ takePicture 빈 프레임");
        esp_camera_fb_return(fb);
        return nullptr;
    }
    
    camera_config.capture_count++;
    return fb;
}

void releasePicture(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

void setCameraQuality(int quality) {
    if (quality < 4) quality = 4;
    if (quality > 63) quality = 63;
    
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
        camera_config.jpeg_quality = quality;
        Serial.printf("📷 JPEG 화질 변경: %d\n", quality);
    }
}

void setCameraResolution(framesize_t size) {
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_framesize(s, size);
        camera_config.frame_size = size;
        Serial.printf("📷 해상도 변경 완료\n");
    }
}

void handleCameraCapture() {
    if (!camera_config.camera_ready || !camera_config.auto_capture) {
        return;
    }
    
    unsigned long now = millis();
    if (now - camera_config.last_capture >= camera_config.capture_interval) {
        Serial.printf("📸 자동 촬영 #%d...\n", camera_config.capture_count + 1);
        
        if (capturePhoto()) {
            camera_config.last_capture = now;
        }
    }
}

void printCameraInfo() {
    Serial.println("📷 카메라 현재 상태:");
    Serial.printf("  준비 상태: %s\n", camera_config.camera_ready ? "준비됨" : "오류");
    Serial.printf("  프레임 크기: %d\n", camera_config.frame_size);
    Serial.printf("  JPEG 품질: %d\n", camera_config.jpeg_quality);
    Serial.printf("  버퍼 개수: %d\n", camera_config.fb_count);
    Serial.printf("  촬영 횟수: %d\n", camera_config.capture_count);
    Serial.printf("  자동 촬영: %s\n", camera_config.auto_capture ? "활성" : "비활성");
}

bool checkCameraHealth() {
    Serial.println("🔍 카메라 건강 상태 점검...");
    
    if (!camera_config.camera_ready) {
        Serial.println("❌ 카메라 준비 상태가 아님");
        return false;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 헬스체크 프레임 획득 실패");
        camera_config.camera_ready = false;
        return false;
    }
    
    if (fb->len == 0) {
        Serial.println("❌ 헬스체크 빈 프레임");
        esp_camera_fb_return(fb);
        camera_config.camera_ready = false;
        return false;
    }
    
    Serial.printf("✅ 카메라 건강 상태 양호 (%d bytes)\n", fb->len);
    esp_camera_fb_return(fb);
    
    return true;
}