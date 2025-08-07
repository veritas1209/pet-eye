// src/streaming.cpp - 서버로 실시간 영상 전송 (HTTP 전용)
#include "streaming.h"
#include "config.h"
#include "sensor_data.h"
#include "wifi_config.h"
#include "camera_manager.h"
#include "http_client.h"
#include <HTTPClient.h>

// 전역 변수 정의
StreamingConfig streaming_config;

void initStreaming() {
    Serial.println("🐾 펫아이 → 서버 영상 전송 시스템 초기화...");
    
    // 스트리밍 설정 초기화
    streaming_config.websocket_enabled = false;
    streaming_config.http_streaming_enabled = true;
    streaming_config.target_fps = 10;               // 10 FPS
    streaming_config.frame_interval = 1000 / streaming_config.target_fps; // 100ms
    streaming_config.last_frame_time = 0;
    streaming_config.max_frame_size = 80000;        // 80KB 최대 프레임 크기
    streaming_config.streaming_active = false;
    streaming_config.frame_count = 0;
    streaming_config.session_start = millis();
    
    Serial.printf("✅ 서버 전송 FPS: %d (프레임 간격: %dms)\n", 
                  streaming_config.target_fps, 
                  streaming_config.frame_interval);
    Serial.println("   📡 ESP32 → 서버로 지속적 영상 전송");
    Serial.println("   🌐 서버팀에서 웹 스트리밍 처리");
}

bool startHTTPStreaming() {
    if (!isWiFiConnected()) {
        Serial.println("❌ WiFi 연결 필요");
        return false;
    }
    
    Serial.println("🚀 서버로 실시간 영상 전송 시작");
    streaming_config.streaming_active = true;
    streaming_config.session_start = millis();
    streaming_config.frame_count = 0;
    
    // 첫 번째 테스트 전송
    Serial.println("📸 첫 프레임 테스트 전송...");
    camera_fb_t* fb = takePicture();
    if (fb) {
        bool success = sendFrameViaHTTP(fb);
        releasePicture(fb);
        
        if (success) {
            Serial.println("✅ 서버 연결 및 영상 전송 테스트 성공!");
            Serial.println("🎬 이제 지속적으로 서버에 영상을 전송합니다...");
            return true;
        }
    }
    
    Serial.println("❌ 서버 연결 테스트 실패");
    return false;
}

void streamFrameToServer(camera_fb_t* fb) {
    if (!streaming_config.streaming_active || !fb) {
        return;
    }
    
    unsigned long now = millis();
    
    // FPS 제어 (정확한 간격으로 전송)
    if (now - streaming_config.last_frame_time < streaming_config.frame_interval) {
        return;
    }
    
    // 프레임 크기 확인
    if (fb->len > streaming_config.max_frame_size) {
        Serial.printf("⚠️  프레임 크기 초과: %d bytes (최대: %d) - 화질 조정 필요\n", 
                     fb->len, streaming_config.max_frame_size);
        return;
    }
    
    // 서버에 HTTP POST로 영상 전송
    if (sendFrameViaHTTP(fb)) {
        streaming_config.last_frame_time = now;
        streaming_config.frame_count++;
        
        // 50프레임마다 통계 출력 (5초마다)
        if (streaming_config.frame_count % 50 == 0) {
            unsigned long uptime = (now - streaming_config.session_start) / 1000;
            float actualFPS = streaming_config.frame_count * 1000.0 / (now - streaming_config.session_start);
            
            Serial.printf("📊 [%lu초] 서버 전송 통계: %d프레임, %.1f FPS, 크기: %d bytes\n",
                         uptime, streaming_config.frame_count, actualFPS, fb->len);
        }
    } else {
        // 전송 실패 시 짧은 대기 후 재시도
        delay(100);
    }
}

bool sendFrameViaHTTP(camera_fb_t* fb) {
    HTTPClient http;
    
    // 서버 URL 설정
    String serverUrl = wifi_config.is_configured ? wifi_config.server_url : SERVER_URL;
    String streamUrl = serverUrl;
    
    // 엔드포인트를 펫아이 전용으로 변경
    streamUrl.replace("/api/sensors", "/api/pet-video");  // 서버팀과 협의된 엔드포인트
    
    http.begin(streamUrl);
    
    // 펫아이 프로젝트 전용 헤더 설정
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("User-Agent", "PetEye-Camera-v1.0");
    http.addHeader("X-Device-ID", device_info.device_id);
    http.addHeader("X-Project", "PetEye");
    http.addHeader("X-Frame-Number", String(streaming_config.frame_count + 1));
    http.addHeader("X-Timestamp", String(millis()));
    http.addHeader("X-FPS", String(streaming_config.target_fps));
    http.addHeader("X-Frame-Size", String(fb->len));
    http.addHeader("X-Resolution", String(fb->width) + "x" + String(fb->height));
    
    // 반려동물 정보 (추가 가능)
    http.addHeader("X-Pet-Type", "dog");  // 나중에 설정 가능하게
    http.addHeader("X-Location", "living-room");  // 카메라 위치
    
    http.setTimeout(3000);  // 3초 타임아웃 (빠른 응답 필요)
    
    // 프레임 전송
    int responseCode = http.POST(fb->buf, fb->len);
    
    String response = "";
    if (responseCode > 0) {
        response = http.getString();
    }
    
    http.end();
    
    bool success = (responseCode == 200 || responseCode == 201);
    
    if (success) {
        // 성공 시 간단한 로그만
        if (streaming_config.frame_count % 100 == 0) {  // 100프레임마다만 출력
            Serial.printf("📡 프레임 #%d 서버 전송 완료 (%d bytes)\n", 
                         streaming_config.frame_count + 1, fb->len);
        }
    } else {
        // 실패 시 상세 로그
        Serial.printf("❌ 서버 전송 실패: HTTP %d", responseCode);
        if (response.length() > 0 && response.length() < 100) {
            Serial.printf(" - %s", response.c_str());
        }
        Serial.println();
        
        // 연속 실패 시 FPS 자동 조정
        static int failCount = 0;
        failCount++;
        if (failCount > 10 && streaming_config.target_fps > 5) {
            setStreamingFPS(streaming_config.target_fps - 2);
            Serial.println("⚠️  연속 실패로 인한 FPS 자동 감소");
            failCount = 0;
        }
    }
    
    return success;
}

void streamSensorDataToServer() {
    // 센서 데이터는 별도로 전송 (기존 방식 활용)
    static unsigned long lastSensorSend = 0;
    unsigned long now = millis();
    
    if (now - lastSensorSend >= 15000) { // 15초마다 센서 데이터
        if (sendSensorData()) {
            Serial.println("📊 센서 데이터도 서버 전송 완료");
        }
        lastSensorSend = now;
    }
}

void processStreamingLoop() {
    // 핵심: 지속적인 영상 프레임 서버 전송
    if (streaming_config.streaming_active && camera_config.camera_ready && isWiFiConnected()) {
        
        camera_fb_t* fb = takePicture();
        if (fb) {
            streamFrameToServer(fb);  // 서버로 실시간 전송
            releasePicture(fb);
        }
    }
    
    // 센서 데이터도 주기적 전송
    streamSensorDataToServer();
    
    // 연결 상태 체크 및 자동 복구
    static unsigned long lastConnectionCheck = 0;
    unsigned long now = millis();
    
    if (now - lastConnectionCheck > 30000) { // 30초마다 연결 체크
        if (!isWiFiConnected()) {
            Serial.println("⚠️  WiFi 연결 끊김 - 스트리밍 일시 중단");
            streaming_config.streaming_active = false;
        } else if (!streaming_config.streaming_active) {
            Serial.println("🔄 WiFi 복구 - 스트리밍 재시작");
            startHTTPStreaming();
        }
        lastConnectionCheck = now;
    }
}

void setStreamingFPS(int fps) {
    if (fps < 2) fps = 2;   // 최소 2 FPS
    if (fps > 15) fps = 15; // 최대 15 FPS (서버 부하 고려)
    
    streaming_config.target_fps = fps;
    streaming_config.frame_interval = 1000 / fps;
    
    Serial.printf("📹 서버 전송 FPS 변경: %d FPS (간격: %dms)\n", 
                  fps, streaming_config.frame_interval);
}

void printStreamingStats() {
    unsigned long sessionTime = millis() - streaming_config.session_start;
    float avgFPS = streaming_config.frame_count * 1000.0 / sessionTime;
    
    Serial.println("\n📊 펫아이 → 서버 전송 통계:");
    Serial.printf("  ⏱️  세션 시간: %lu초 (%lu분)\n", 
                  sessionTime / 1000, sessionTime / 60000);
    Serial.printf("  📸 총 전송 프레임: %d개\n", streaming_config.frame_count);
    Serial.printf("  📈 평균 FPS: %.2f\n", avgFPS);
    Serial.printf("  🎯 목표 FPS: %d\n", streaming_config.target_fps);
    Serial.printf("  📡 전송 방식: HTTP POST (펫아이 프로젝트)\n");
    Serial.printf("  🔄 전송 상태: %s\n", streaming_config.streaming_active ? "활성" : "중지");
    Serial.printf("  🌐 서버 URL: %s\n", 
                  wifi_config.is_configured ? wifi_config.server_url.c_str() : SERVER_URL);
    Serial.println("  📝 역할: ESP32(영상전송) → 서버(웹스트리밍)");
    Serial.println();
}