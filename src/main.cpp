// src/main.cpp - 센서 데이터 서버 전송 버전
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

// WiFi 설정
const char* ssid = "PRO";
const char* password = "propro123";

// 서버 설정
const char* server_url = "http://192.168.0.27:3000/api/sensors"; // 서버 IP와 포트를 실제 값으로 변경하세요
const unsigned long SEND_INTERVAL = 10000; // 10초마다 전송

// 핀 설정
#define TEMP_PIN 4
#define I2C_SDA 17
#define I2C_SCL 18

// HTTP 클라이언트
HTTPClient http;

// 센서 데이터 구조체
struct SensorData {
    float temperature;
    int i2c_count;
    unsigned long last_temp_read;
    unsigned long last_i2c_scan;
    unsigned long last_send;
    bool data_ready;
    bool initial_scan_done;
} sensors;

// 디바이스 정보
struct DeviceInfo {
    String device_id;
    String chip_model;
    int chip_revision;
    int cpu_freq;
    String mac_address;
} device_info;

// 온도 읽기 함수
void readTemperature() {
    unsigned long now = millis();
    if (now - sensors.last_temp_read >= 2000) { // 2초마다 업데이트
        int raw = analogRead(TEMP_PIN);
        float voltage = raw * (3.3 / 4095.0);
        
        // SEN050007 센서용 LM335 공식
        float temp_lm335 = (voltage * 1000 - 2732) / 10.0;
        sensors.temperature = temp_lm335;
        
        // 상식적인 범위 체크 (-10°C ~ 85°C)
        if (sensors.temperature < -10 || sensors.temperature > 85) {
            Serial.printf("온도 값 이상: %.1f°C (Raw: %d, V: %.3f)\n", 
                         sensors.temperature, raw, voltage);
        }
        
        sensors.last_temp_read = now;
        sensors.data_ready = true;
        
        Serial.printf("온도 읽기: %.1f°C\n", sensors.temperature);
    }
}

// I2C 디바이스 스캔 (비동기식 - 한 번에 하나씩)
uint8_t current_i2c_addr = 0x08;
bool i2c_scan_complete = false;

void scanI2C() {
    static unsigned long last_addr_check = 0;
    unsigned long now = millis();
    
    // 초기 스캔이 완료되지 않았거나, 5분마다 재스캔
    if (!i2c_scan_complete || (i2c_scan_complete && now - sensors.last_i2c_scan >= 300000)) {
        
        // 10ms마다 주소 하나씩 체크 (비블로킹)
        if (now - last_addr_check >= 10) {
            Wire.setTimeout(5); // 타임아웃 단축
            Wire.beginTransmission(current_i2c_addr);
            
            if (Wire.endTransmission() == 0) {
                if (!i2c_scan_complete) {
                    Serial.printf("I2C 발견: 0x%02X\n", current_i2c_addr);
                }
                sensors.i2c_count++;
            }
            
            current_i2c_addr++;
            last_addr_check = now;
            
            // 스캔 완료 체크
            if (current_i2c_addr >= 0x78 || sensors.i2c_count >= 10) {
                if (!i2c_scan_complete) {
                    Serial.printf("✅ I2C 스캔 완료: %d개 디바이스 발견\n", sensors.i2c_count);
                    i2c_scan_complete = true;
                }
                sensors.last_i2c_scan = now;
                current_i2c_addr = 0x08; // 다음 스캔을 위해 리셋
                sensors.data_ready = true;
            }
        }
    }
}

// JSON 데이터 생성 (수동으로 문자열 생성)
String createSensorJSON() {
    String json = "{";
    
    // 디바이스 정보
    json += "\"device_id\":\"" + device_info.device_id + "\",";
    json += "\"device_name\":\"T-Camera-S3\",";
    json += "\"location\":\"Lab-01\","; // 필요에 따라 수정
    
    // 센서 데이터
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"temperature\":" + String(sensors.temperature, 1) + ",";
    json += "\"i2c_devices\":" + String(sensors.i2c_count) + ",";
    
    // 시스템 정보
    json += "\"system\":{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz());
    json += "}";
    
    json += "}";
    return json;
}

// 서버에 데이터 전송
bool sendSensorData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi 연결되지 않음 - 전송 건너뜀");
        return false;
    }
    
    String jsonData = createSensorJSON();
    
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-T-Camera-S3");
    http.setTimeout(5000); // 5초 타임아웃
    
    Serial.printf("서버로 전송 중: %s\n", server_url);
    Serial.printf("데이터 크기: %d bytes\n", jsonData.length());
    
    int httpResponseCode = http.POST(jsonData);
    
    bool success = false;
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("서버 응답 코드: %d\n", httpResponseCode);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("✅ 데이터 전송 성공!");
            Serial.printf("응답: %s\n", response.c_str());
            success = true;
        } else {
            Serial.printf("❌ 서버 오류: %d\n", httpResponseCode);
            Serial.printf("응답: %s\n", response.c_str());
        }
    } else {
        Serial.printf("❌ HTTP 전송 실패: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    return success;
}

// WiFi 연결 함수
void connectWiFi() {
    Serial.printf("WiFi 연결 시도: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    
    WiFi.begin(ssid, password);
    Serial.print("연결 중");
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 30000) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d]", attempts);
        }
        
        // 연결 시도 중에도 센서 읽기
        if (attempts % 4 == 0) {
            readTemperature();
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi 연결 성공!");
        Serial.printf("IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("신호 강도: %d dBm\n", WiFi.RSSI());
        Serial.printf("게이트웨이: %s\n", WiFi.gatewayIP().toString().c_str());
    } else {
        Serial.println("❌ WiFi 연결 실패!");
        Serial.printf("상태 코드: %d\n", WiFi.status());
    }
}

// WiFi 재연결 처리
void handleWiFiReconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= 30000) { // 30초마다 재연결 시도
            Serial.println("WiFi 재연결 시도...");
            WiFi.begin(ssid, password);
            lastReconnectAttempt = now;
        }
    }
}

// 초기화 함수
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== T-Camera S3 센서 클라이언트 시작 ===");
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize()/1024/1024);
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap()/1024);
    Serial.printf("PSRAM: %s\n", psramFound() ? "Available" : "Not found");
    
    // 디바이스 정보 설정
    device_info.device_id = WiFi.macAddress();
    device_info.chip_model = ESP.getChipModel();
    device_info.chip_revision = ESP.getChipRevision();
    device_info.cpu_freq = ESP.getCpuFreqMHz();
    device_info.mac_address = WiFi.macAddress();
    
    Serial.printf("디바이스 ID: %s\n", device_info.device_id.c_str());
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    Serial.println("I2C 초기화 완료");
    
    // ADC 설정
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    Serial.println("ADC 설정 완료");
    
    // 초기 센서 읽기
    sensors.data_ready = false;
    sensors.last_send = 0;
    sensors.initial_scan_done = false;
    sensors.i2c_count = 0; // 초기값
    
    readTemperature();
    // scanI2C(); // 제거 - setup에서 바로 스캔하지 않음
    
    Serial.printf("초기 센서 값: %.1f°C\n", sensors.temperature);
    Serial.println("I2C 스캔은 백그라운드에서 진행됩니다...");
    
    // WiFi 연결
    connectWiFi();
    
    Serial.printf("서버 URL: %s\n", server_url);
    Serial.printf("전송 간격: %d초\n", SEND_INTERVAL / 1000);
    Serial.println("=== 초기화 완료 ===\n");
}

// 메인 루프
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
            sensors.data_ready = false; // 전송 성공 시 플래그 리셋
        }
        sensors.last_send = now;
    }
    
    // 상태 출력 (5초마다)
    static unsigned long lastStatus = 0;
    if (now - lastStatus >= 5000) {
        String i2c_status = i2c_scan_complete ? String(sensors.i2c_count) : "스캔중";
        
        Serial.printf("[%lu] WiFi: %s | T: %.1f°C | I2C: %s | MEM: %dKB | 다음전송: %ds\n",
                      now / 1000,
                      WiFi.status() == WL_CONNECTED ? "연결됨" : "연결안됨",
                      sensors.temperature,
                      i2c_status.c_str(),
                      ESP.getFreeHeap() / 1024,
                      (SEND_INTERVAL - (now - sensors.last_send)) / 1000);
        lastStatus = now;
    }
    
    delay(100); // 100ms 딜레이
}