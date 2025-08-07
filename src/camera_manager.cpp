// src/camera_manager.cpp - T-Camera S3 카메라 관리 구현
#include "camera_manager.h"
#include "config.h"

// 카메라 설정 전역 변수
CameraConfig camera_config;

bool initCamera() {
    Serial.println("📷 T-Camera S3 카메라 초기화 중...");
    
    // 카메라 설정 구조체 초기화
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
    // deprecated 경고 수정: 새로운 필드명 사용
    config.pin_sccb_sda = SIOD_GPIO_NUM;  // pin_sscb_sda → pin_sccb_sda
    config.pin_sccb_scl = SIOC_GPIO_NUM;  // pin_sscb_scl → pin_sccb_scl
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;  // 20MHz
    config.pixel_format = PIXFORMAT_JPEG;
    
    // ESP32-S3의 PSRAM 사용
    if(psramFound()){
        Serial.println("✅ PSRAM 발견 - 고화질 설정 적용");
        config.frame_size = FRAMESIZE_SVGA;     // 800x600 (스트리밍에 적합)
        config.jpeg_quality = 12;              // 적당한 화질
        config.fb_count = 2;                   // 프레임 버퍼 2개
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    } else {
        Serial.println("⚠️  PSRAM 없음 - 저화질 설정 적용");
        config.frame_size = FRAMESIZE_VGA;     // 640x480
        config.jpeg_quality = 15;
        config.fb_count = 1;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    
    // 카메라 초기화
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("❌ 카메라 초기화 실패: 0x%x\n", err);
        camera_config.camera_ready = false;
        return false;
    }
    
    // 센서 최적화 설정
    sensor_t* s = esp_camera_sensor_get();
    if (s != NULL) {
        // 펫아이 프로젝트에 적합한 카메라 설정
        s->set_brightness(s, 0);     // 밝기 0 (기본)
        s->set_contrast(s, 0);       // 대비 0 (기본)
        s->set_saturation(s, -1);    // 채도 약간 낮춤 (자연스러운 색감)
        s->set_special_effect(s, 0); // 특수 효과 없음
        s->set_whitebal(s, 1);       // 화이트밸런스 자동
        s->set_awb_gain(s, 1);       // AWB 게인 활성화
        s->set_exposure_ctrl(s, 1);  // 자동 노출
        s->set_aec2(s, 0);           // AEC2 비활성화
        s->set_gain_ctrl(s, 1);      // 게인 제어 활성화
        s->set_agc_gain(s, 0);       // AGC 게인 0
        s->set_bpc(s, 0);            // BPC 비활성화
        s->set_wpc(s, 1);            // WPC 활성화
        s->set_raw_gma(s, 1);        // RAW GMA 활성화
        s->set_lenc(s, 1);           // 렌즈 보정 활성화
        s->set_hmirror(s, 0);        // 수평 미러 비활성화
        s->set_vflip(s, 0);          // 수직 플립 비활성화
        s->set_dcw(s, 1);            // DCW 활성화
        s->set_colorbar(s, 0);       // 컬러바 비활성화
        
        Serial.println("✅ 카메라 센서 최적화 완료");
    }
    
    // 내부 설정 초기화
    camera_config.frame_size = config.frame_size;
    camera_config.pixel_format = config.pixel_format;
    camera_config.jpeg_quality = config.jpeg_quality;
    camera_config.fb_count = config.fb_count;
    camera_config.auto_capture = true;
    camera_config.capture_interval = CAMERA_CAPTURE_INTERVAL;
    camera_config.last_capture = 0;
    camera_config.camera_ready = true;
    camera_config.capture_count = 0;
    
    Serial.println("✅ 카메라 초기화 성공!");
    printCameraInfo();
    
    // 첫 번째 테스트 촬영
    Serial.println("📸 테스트 촬영 중...");
    if (capturePhoto()) {
        Serial.println("✅ 테스트 촬영 성공!");
        return true;
    } else {
        Serial.println("⚠️  테스트 촬영 실패, 하지만 계속 진행");
        return true;  // 초기화는 성공했으므로 true 반환
    }
}

bool capturePhoto() {
    if (!camera_config.camera_ready) {
        Serial.println("❌ 카메라가 준비되지 않음");
        return false;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 카메라 프레임 버퍼 획득 실패");
        return false;
    }
    
    Serial.printf("📸 촬영 완료 - 크기: %d bytes, 해상도: %dx%d\n", 
                  fb->len, fb->width, fb->height);
    
    // 프레임 버퍼 해제
    esp_camera_fb_return(fb);
    camera_config.capture_count++;
    
    return true;
}

camera_fb_t* takePicture() {
    if (!camera_config.camera_ready) {
        Serial.println("❌ 카메라가 준비되지 않음");
        return nullptr;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 사진 촬영 실패");
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
    if (quality < 4) quality = 4;   // 최고 화질
    if (quality > 63) quality = 63; // 최저 화질
    
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
        camera_config.jpeg_quality = quality;
        Serial.printf("📷 JPEG 화질 설정: %d\n", quality);
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
        
        Serial.printf("📸 자동 촬영 시작 (촬영 #%d)...\n", camera_config.capture_count + 1);
        
        if (capturePhoto()) {
            camera_config.last_capture = now;
        } else {
            Serial.println("❌ 자동 촬영 실패");
        }
    }
}

void printCameraInfo() {
    Serial.println("📷 카메라 정보:");
    Serial.printf("  해상도: %s\n", camera_config.frame_size == FRAMESIZE_SVGA ? "SVGA (800x600)" : "VGA (640x480)");
    Serial.printf("  JPEG 화질: %d\n", camera_config.jpeg_quality);
    Serial.printf("  프레임 버퍼: %d개\n", camera_config.fb_count);
    Serial.printf("  PSRAM: %s\n", psramFound() ? "사용 가능" : "없음");
    Serial.printf("  펫아이 최적화: 활성화\n");
}

bool checkCameraHealth() {
    Serial.println("🔍 카메라 상태 점검 중...");
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("❌ 프레임 버퍼를 가져올 수 없습니다");
        camera_config.camera_ready = false;
        return false;
    }
    
    if (fb->len == 0 || fb->buf == nullptr) {
        Serial.println("❌ 유효하지 않은 이미지 데이터");
        esp_camera_fb_return(fb);
        camera_config.camera_ready = false;
        return false;
    }
    
    Serial.printf("✅ 카메라 상태 정상 - 이미지 크기: %d bytes\n", fb->len);
    esp_camera_fb_return(fb);
    
    camera_config.camera_ready = true;
    return true;
}