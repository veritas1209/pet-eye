// src/main.cpp - 펫아이 프로젝트 메인 (실시간 스트리밍)
#include <Arduino.h>

// 각 모듈 헤더 포함
#include "config.h"
#include "sensor_data.h"
#include "wifi_manager.h"
#include "ds18b20_sensor.h"
#include "i2c_scanner.h"
#include "http_client.h"
#include "camera_manager.h"
#include "streaming.h"

void printSystemInfo() {
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("PSRAM: %s\n", psramFound() ? "사용 가능" : "없음");
}

void printStatus() {
    static unsigned long lastStatus = 0;
    unsigned long now = millis();
    
    if (now - lastStatus >= STATUS_PRINT_INTERVAL) {
        String temp_status = sensors.temp_valid ? 
                           String(sensors.temperature, 2) + "°C" : "오류";
        String camera_status = camera_config.camera_ready ? "정상" : "오류";
        String stream_status = streaming_config.streaming_active ? 
                             String(streaming_config.frame_count) + "프레임" : "중지됨";
        
        Serial.printf("[%lu] 🐾 펫아이 상태 | WiFi: %s | 온도: %s | 카메라: %s | 스트림: %s\n",
                      now / 1000,
                      isWiFiConnected() ? "연결" : "끊김",
                      temp_status.c_str(),
                      camera_status.c_str(),
                      stream_status.c_str());
                      
        // WebSocket 연결 상태
        if (webSocket.isConnected()) {
            float currentFPS = 1000.0 / streaming_config.frame_interval;
            Serial.printf("     📡 실시간 스트리밍: %.1f FPS | 메모리: %dKB\n", 
                         currentFPS, ESP.getFreeHeap() / 1024);
        } else {
            Serial.printf("     ❌ 서버 연결 끊어짐 | 메모리: %dKB\n", ESP.getFreeHeap() / 1024);
        }
                      
        lastStatus = now;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n🐾 ===== 펫아이 (PetEye) 시스템 시작 =====");
    Serial.println("     반려동물 실시간 모니터링 시스템");
    Serial.println("=========================================");
    printSystemInfo();
    
    // 1. 데이터 구조체 초기화
    initSensorData();
    initDeviceInfo();
    
    // 2. 카메라 초기화 (최우선)
    bool camera_ok = initCamera();
    if (camera_ok) {
        Serial.println("✅ 📷 카메라 모듈 준비 완료");
        // 펫 모니터링에 적합한 설정
        setCameraResolution(FRAMESIZE_SVGA);  // 800x600 (스트리밍에 적합)
        setCameraQuality(12);                 // 적당한 화질 (스트리밍 속도 우선)
    } else {
        Serial.println("❌ 카메라 초기화 실패 - 시스템 재시작 필요");
        ESP.restart();
    }
    
    // 3. 센서 초기화
    initI2C();
    bool ds18b20_ok = initDS18B20();
    if (ds18b20_ok) {
        readTemperature();
        Serial.printf("✅ 🌡️  초기 온도: %.1f°C\n", sensors.temperature);
        
        // 반려동물 체온 범위 안내
        if (sensors.temperature > 39.0) {
            Serial.println("⚠️  높은 온도 감지 - 반려동물 발열 주의!");
        } else if (sensors.temperature < 36.0) {
            Serial.println("⚠️  낮은 온도 감지 - 반려동물 체온 저하 주의!");
        }
    }
    
    // 4. WiFi 연결
    Serial.println("📶 WiFi 연결 중...");
    connectWiFi();
    
    if (isWiFiConnected()) {
        Serial.println("✅ 네트워크 연결 완료");
        
        // 5. 스트리밍 시스템 초기화
        initStreaming();
        
        // 6. 서버 연결 및 스트리밍 시작
        if (startWebSocketStreaming()) {
            Serial.println("✅ 🎥 실시간 스트리밍 시작!");
            Serial.println("     서버에서 펫 영상을 실시간으로 확인할 수 있습니다.");
        } else {
            Serial.println("⚠️  WebSocket 연결 실패 - HTTP 모드로 동작");
        }
    } else {
        Serial.println("❌ WiFi 연결 실패 - 오프라인 모드");
    }
    
    Serial.println("\n🐾 ===== 펫아이 시스템 준비 완료 =====");
    Serial.println("     반려동물 모니터링을 시작합니다...");
    Serial.println("=====================================\n");
}

void loop() {
    // 1. WiFi 연결 관리
    handleWiFiReconnection();
    
    // 2. 센서 데이터 수집
    readTemperature();
    scanI2C();
    
    // 3. 실시간 스트리밍 처리 (핵심 기능)
    processStreamingLoop();
    
    // 4. 주기적 데이터 백업 전송 (WebSocket 보조)
    static unsigned long lastBackupSend = 0;
    unsigned long now = millis();
    if (now - lastBackupSend >= 60000) { // 1분마다
        if (isWiFiConnected() && sensors.data_ready) {
            // HTTP로도 센서 데이터 백업 전송
            sendSensorData();
            lastBackupSend = now;
        }
    }
    
    // 5. 상태 출력
    printStatus();
    
    // 6. 시스템 안정성을 위한 최소 지연
    delay(10);
    
    // 7. 메모리 부족 시 재시작
    if (ESP.getFreeHeap() < 20000) { // 20KB 미만
        Serial.println("⚠️  메모리 부족 - 시스템 재시작");
        ESP.restart();
    }
} // src/main.cpp - 메인 애플리케이션 (리팩토링됨)
#include <Arduino.h>

// 각 모듈 헤더 포함
#include "config.h"
#include "sensor_data.h"
#include "wifi_manager.h"
#include "ds18b20_sensor.h"
#include "i2c_scanner.h"
#include "http_client.h"

void printSystemInfo() {
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
}

void printStatus() {
    static unsigned long lastStatus = 0;
    unsigned long now = millis();
    
    if (now - lastStatus >= STATUS_PRINT_INTERVAL) {
        String temp_status = sensors.temp_valid ? 
                           String(sensors.temperature, 2) + "°C" : "오류";
        String i2c_status = i2c_scan_complete ? String(sensors.i2c_count) : "스캔중";
        
        Serial.printf("[%lu] WiFi: %s | DS18B20: %s | I2C: %s | MEM: %dKB | 다음전송: %ds\n",
                      now / 1000,
                      isWiFiConnected() ? "연결됨" : "연결안됨",
                      temp_status.c_str(),
                      i2c_status.c_str(),
                      ESP.getFreeHeap() / 1024,
                      (SEND_INTERVAL - (now - sensors.last_send)) / 1000);
        lastStatus = now;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== T-Camera S3 + DS18B20 센서 시작 ===");
    printSystemInfo();
    
    // 데이터 구조체 초기화
    initSensorData();
    initDeviceInfo();
    
    // I2C 초기화
    initI2C();
    
    // DS18B20 초기화
    bool ds18b20_ok = initDS18B20();
    
    if (ds18b20_ok) {
        readTemperature(); // 첫 온도 읽기
        Serial.printf("초기 온도: %.2f°C\n", sensors.temperature);
    }
    
    Serial.println("I2C 스캔은 백그라운드에서 진행됩니다...");
    
    // WiFi 연결
    connectWiFi();
    
    Serial.printf("서버 URL: %s\n", SERVER_URL);
    Serial.printf("전송 간격: %d초\n", SEND_INTERVAL / 1000);
    Serial.println("=== 초기화 완료 ===\n");
}

void loop() {
    // WiFi 연결 상태 확인 및 재연결
    handleWiFiReconnection();
    
    // 센서 데이터 읽기
    readTemperature();
    scanI2C();
    
    // 서버에 데이터 전송 (주기적)
    unsigned long now = millis();
    if (sensors.data_ready && (now - sensors.last_send >= SEND_INTERVAL)) {
        if (sendSensorData()) {
            sensors.data_ready = false;
        }
        sensors.last_send = now;
    }
    
    // 상태 출력
    printStatus();
    
    delay(100);
}