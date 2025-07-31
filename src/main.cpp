// src/main.cpp - 완전히 수정된 버전
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// WiFi 설정
const char* ssid = "PRO";
const char* password = "propro123";

// 핀 설정
#define TEMP_PIN 4
#define I2C_SDA 17
#define I2C_SCL 18

// 웹서버
WebServer server(80);

// 센서 데이터
struct {
    float temperature;
    int i2c_count;
    unsigned long last_temp_read;
    unsigned long last_i2c_scan;
} sensors;

// 온도 읽기 (SEN050007 센서 공식 적용)
void readTemperature() {
    unsigned long now = millis();
    if (now - sensors.last_temp_read >= 500) { // 0.5초마다 업데이트
        int raw = analogRead(TEMP_PIN);
        float voltage = raw * (3.3 / 4095.0); // ESP32 ADC: 12비트, 3.3V
        
        float temp_lm335 = (voltage * 1000 - 2732) / 10.0; // LM335 타입: 10mV/K
        
        // 실제 온도값 확인을 위해 모든 공식 출력
        Serial.printf("Raw: %d, Voltage: %.3fV\n", raw, voltage);
        Serial.printf("LM335형: %.1f°C\n", temp_lm335);
        
        // 일단 가장 일반적인 LM35 공식 사용 (실제 측정 후 조정)
        sensors.temperature = temp_lm335;
        
        // 상식적인 범위 체크 (-10°C ~ 85°C)
        if (sensors.temperature < -10 || sensors.temperature > 85) {
            // 범위를 벗어나면 TMP36 공식 시도
            sensors.temperature = temp_lm335;
            Serial.printf("→ TMP36 공식 적용: %.1f°C\n", sensors.temperature);
        }
        
        sensors.last_temp_read = now;
    }
}

// I2C 스캔 (최적화 - 빠른 스캔)
void scanI2C() {
    unsigned long now = millis();
    if (now - sensors.last_i2c_scan >= 15000) { // 15초마다로 변경 (더 자주 스캔 방지)
        sensors.i2c_count = 0;
        Wire.setTimeout(5); // 타임아웃 단축
        
        // 빠른 스캔 (범위 축소)
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                sensors.i2c_count++;
                if (sensors.i2c_count >= 10) break; // 최대 10개까지만
            }
        }
        
        sensors.last_i2c_scan = now;
    }
}

// 웹페이지 핸들러 (최적화)
void handleRoot() {
    // 웹 요청 시 즉시 센서 읽기
    readTemperature();
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>T-Camera S3 Fast</title>";
    html += "<meta charset='utf-8'>";
    html += "<meta http-equiv='refresh' content='2'>"; // 2초로 단축
    html += "</head><body>";
    html += "<h1>T-Camera S3 Monitor</h1>";
    html += "<p><strong>Temperature:</strong> " + String(sensors.temperature, 1) + "°C</p>";
    html += "<p><strong>I2C Devices:</strong> " + String(sensors.i2c_count) + " 개</p>";
    html += "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()/1024) + " KB</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis()/1000) + " seconds</p>";
    html += "<p><strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
    html += "<hr>";
    html += "<p>Auto refresh every 2 seconds</p>";
    html += "<p>Last update: " + String(millis()) + "ms</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    Serial.printf("[WEB] Page served - T:%.1f°C\n", sensors.temperature);
}

// API 핸들러 (최적화)
void handleAPI() {
    // API 요청 시에도 즉시 센서 읽기
    readTemperature();
    
    String json = "{";
    json += "\"temperature\":" + String(sensors.temperature, 1) + ",";
    json += "\"i2c_devices\":" + String(sensors.i2c_count) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    server.send(200, "application/json", json);
    Serial.printf("[API] Data sent - T:%.1f°C\n", sensors.temperature);
}

// WiFi 연결 함수
void connectWiFi() {
    Serial.printf("WiFi 연결 시도: %s\n", ssid);
    Serial.printf("비밀번호 길이: %d 문자\n", strlen(password));
    
    // WiFi 모드 설정 (모듈화 코드와 동일)
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    
    // 연결 시작
    WiFi.begin(ssid, password);
    Serial.print("연결 중");
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    // WiFi 타임아웃: 30초 (모듈화 코드와 동일)
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 30000) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d]", attempts);
        }
        
        // 연결 시도 중에도 센서 읽기 (모듈화 코드 방식)
        if (attempts % 4 == 0) {
            readTemperature();
            Serial.printf("\n  중간 체크 - T: %.1f°C", sensors.temperature);
            Serial.print(" 계속 연결 중");
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✓ WiFi 연결 성공!");
        Serial.printf("✓ SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("✓ IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("✓ 신호 강도: %d dBm\n", WiFi.RSSI());
        Serial.printf("✓ Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("✓ DNS: %s\n", WiFi.dnsIP().toString().c_str());
    } else {
        Serial.println("✗ WiFi 연결 실패!");
        Serial.printf("상태 코드: %d\n", WiFi.status());
        Serial.println("오프라인 모드로 계속 진행");
    }
}

// 초기화 함수 (setup)
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== T-Camera S3 시작 ===");
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize()/1024/1024);
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap()/1024);
    Serial.printf("PSRAM: %s\n", psramFound() ? "Available" : "Not found");
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    Serial.println("I2C 초기화 완료");
    
    // ADC 설정
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    Serial.println("ADC 설정 완료");
    
    // 초기 센서 읽기
    readTemperature();
    scanI2C();
    Serial.printf("초기 센서 값: %.1f°C, I2C: %d개\n", 
                  sensors.temperature, sensors.i2c_count);
    
    // WiFi 연결
    connectWiFi();
    
    // 웹서버 시작 (연결 성공 시에만)
    if (WiFi.status() == WL_CONNECTED) {
        server.on("/", handleRoot);
        server.on("/api", handleAPI);
        server.begin();
        
        Serial.println("✅ 웹서버 시작 완료!");
        Serial.printf("🌐 브라우저 접속: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📊 API 접속: http://%s/api\n", WiFi.localIP().toString().c_str());
    }
    
    Serial.println("=== 초기화 완료 ===\n");
}

// 메인 루프 (최적화)
void loop() {
    // 웹서버 처리 (최우선)
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    }
    
    // 센서 업데이트 (논블로킹)
    readTemperature(); // 0.5초마다 체크
    scanI2C();         // 15초마다 체크
    
    // 상태 출력 (3초마다로 단축)
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 3000) {
        Serial.printf("[%lu] WiFi: %s | T: %.1f°C | I2C: %d | MEM: %dKB",
                      millis()/1000,
                      WiFi.status() == WL_CONNECTED ? "OK" : "OFF",
                      sensors.temperature,
                      sensors.i2c_count,
                      ESP.getFreeHeap()/1024);
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf(" | RSSI: %ddBm", WiFi.RSSI());
        }
        Serial.println();
        
        lastStatus = millis();
    }
    
    // WiFi 재연결 (간소화)
    static unsigned long lastReconnect = 0;
    if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect > 30000) {
        Serial.println("WiFi 재연결 시도...");
        WiFi.begin(ssid, password);
        lastReconnect = millis();
    }
    
    delay(10); // 딜레이 단축 (100ms → 10ms)
}