// src/main.cpp - 간단한 웹서버 버전
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// WiFi 설정
const char* ssid = "PRO";
const char* password = "your_password";  // 실제 비밀번호로 변경

// 핀 설정
#define I2C_SDA 17
#define I2C_SCL 18
#define TEMP_SENSOR_PIN 4

// 웹서버
WebServer server(80);

// 센서 데이터
struct {
    float temperature;
    int i2cDeviceCount;
    unsigned long lastUpdate;
} sensorData;

void scanI2C() {
    Serial.println("I2C 스캔 중...");
    sensorData.i2cDeviceCount = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("✓ I2C 장치: 0x%02X\n", address);
            sensorData.i2cDeviceCount++;
        }
    }
    Serial.printf("총 %d개 발견\n", sensorData.i2cDeviceCount);
}

void readTemperature() {
    int raw = analogRead(TEMP_SENSOR_PIN);
    float voltage = raw * (3.3 / 4095.0);
    sensorData.temperature = (voltage - 0.5) * 100.0;
    sensorData.lastUpdate = millis();
    
    Serial.printf("온도: Raw=%d, V=%.3f, T=%.1f°C\n", 
                  raw, voltage, sensorData.temperature);
}

void handleRoot() {
    Serial.println("웹 요청 받음!");
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32-S3 Test</title>";
    html += "<meta charset='utf-8'>";
    html += "<style>";
    html += "body{font-family:Arial;margin:40px;background:#f0f0f0}";
    html += ".container{background:white;padding:30px;border-radius:10px;max-width:600px;margin:0 auto}";
    html += ".status{background:#d4edda;color:#155724;padding:15px;margin:10px 0;border-radius:5px}";
    html += ".info{background:#e9ecef;padding:12px;margin:8px 0;border-radius:4px}";
    html += ".sensor{background:#f8f9fa;padding:20px;margin:15px 0;border-radius:8px;border-left:5px solid #007bff}";
    html += ".value{font-size:28px;color:#007bff;font-weight:bold;margin:10px 0}";
    html += "button{padding:12px 24px;background:#007bff;color:white;border:none;border-radius:6px;cursor:pointer;margin:8px;font-size:16px}";
    html += "button:hover{background:#0056b3}";
    html += "h1{text-align:center;color:#333;margin-bottom:30px}";
    html += "h2{color:#555;border-bottom:3px solid #007bff;padding-bottom:8px}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🚀 ESP32-S3 실시간 모니터링</h1>";
    
    // 상태
    html += "<div class='status'>✅ 시스템 정상 동작</div>";
    html += "<div class='status'>🌐 WiFi 연결: " + WiFi.SSID() + "</div>";
    html += "<div class='status'>📡 IP 주소: " + WiFi.localIP().toString() + "</div>";
    
    // 시스템 정보
    html += "<h2>💻 시스템 정보</h2>";
    html += "<div class='info'><strong>Chip:</strong> " + String(ESP.getChipModel()) + "</div>";
    html += "<div class='info'><strong>CPU:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</div>";
    html += "<div class='info'><strong>Flash:</strong> " + String(ESP.getFlashChipSize()/1024/1024) + " MB</div>";
    html += "<div class='info'><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()/1024) + " KB</div>";
    html += "<div class='info'><strong>PSRAM:</strong> " + String(psramFound() ? "Available" : "Not found") + "</div>";
    html += "<div class='info'><strong>Uptime:</strong> " + String(millis()/1000) + " seconds</div>";
    
    // 센서 데이터
    html += "<h2>📊 실시간 센서 데이터</h2>";
    
    html += "<div class='sensor'>";
    html += "<h3>🌡️ 온도 센서</h3>";
    html += "<div class='value'>" + String(sensorData.temperature, 1) + "°C</div>";
    html += "<div>Raw 값: " + String(analogRead(TEMP_SENSOR_PIN)) + "</div>";
    html += "<div>업데이트: " + String((millis() - sensorData.lastUpdate)/1000) + "초 전</div>";
    html += "</div>";
    
    html += "<div class='sensor'>";
    html += "<h3>🔌 I2C 장치</h3>";
    html += "<div class='value'>" + String(sensorData.i2cDeviceCount) + " 개</div>";
    html += "<div>실시간 스캔 결과</div>";
    html += "</div>";
    
    // 컨트롤
    html += "<h2>🎮 제어판</h2>";
    html += "<button onclick='location.reload()'>🔄 전체 새로고침</button>";
    html += "<button onclick='updateData()'>📊 데이터 갱신</button>";
    html += "<button onclick='window.open(\"/api\")'>🔗 API 보기</button>";
    
    // JavaScript
    html += "<script>";
    html += "function updateData() {";
    html += "  fetch('/api').then(r=>r.text()).then(d=>{";
    html += "    alert('데이터 업데이트됨: ' + d);";
    html += "    location.reload();";
    html += "  }).catch(e=>alert('오류: '+e));";
    html += "}";
    html += "setInterval(updateData, 10000);"; // 10초마다 자동 갱신
    html += "</script>";
    
    html += "</div></body></html>";
    
    server.send(200, "text/html", html);
    Serial.println("응답 전송 완료!");
}

void handleAPI() {
    Serial.println("API 요청 받음!");
    
    // 실시간 데이터 업데이트
    readTemperature();
    scanI2C();
    
    String json = "{";
    json += "\"temperature\":" + String(sensorData.temperature, 2) + ",";
    json += "\"i2cDevices\":" + String(sensorData.i2cDeviceCount) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";
    
    server.send(200, "application/json", json);
    Serial.println("API 응답 완료!");
}

void handleNotFound() {
    Serial.printf("404 요청: %s\n", server.uri().c_str());
    
    String message = "404 - 페이지 없음\n\n";
    message += "요청 URI: " + server.uri() + "\n";
    message += "\n사용 가능한 페이지:\n";
    message += "/ - 메인 페이지\n";
    message += "/api - API 데이터\n";
    
    server.send(404, "text/plain", message);
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    
    Serial.println("\n=== ESP32-S3 간단 웹서버 테스트 ===");
    Serial.printf("Chip: %s\n", ESP.getChipModel());
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("I2C 초기화 완료");
    
    // 온도 센서 핀
    pinMode(TEMP_SENSOR_PIN, INPUT);
    Serial.println("센서 핀 설정 완료");
    
    // 초기 센서 읽기
    readTemperature();
    scanI2C();
    
    // WiFi 연결
    Serial.printf("WiFi 연결 중: %s\n", ssid);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi 연결 성공!");
        Serial.printf("IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("신호 강도: %d dBm\n", WiFi.RSSI());
        
        // 웹서버 시작
        Serial.println("\n--- 웹서버 시작 ---");
        server.on("/", handleRoot);
        server.on("/api", handleAPI);  
        server.onNotFound(handleNotFound);
        server.begin();
        
        Serial.println("✅ 웹서버 시작 완료!");
        Serial.printf("🌐 브라우저 접속: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.println("================================");
    } else {
        Serial.println("\n❌ WiFi 연결 실패!");
    }
}

void loop() {
    // WiFi 연결 상태 확인
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    } else {
        // 재연결 시도
        static unsigned long lastReconnect = 0;
        if (millis() - lastReconnect > 30000) {
            Serial.println("WiFi 재연결 시도...");
            WiFi.begin(ssid, password);
            lastReconnect = millis();
        }
    }
    
    // 2초마다 센서 업데이트
    static unsigned long lastSensorUpdate = 0;
    if (millis() - lastSensorUpdate > 2000) {
        readTemperature();
        lastSensorUpdate = millis();
    }
    
    // 10초마다 I2C 스캔
    static unsigned long lastI2CScan = 0;
    if (millis() - lastI2CScan > 10000) {
        scanI2C();
        lastI2CScan = millis();
    }
    
    // 5초마다 상태 출력
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        Serial.printf("[%lu] WiFi: %s, Heap: %d KB, 웹서버: 대기중\n",
                      millis() / 1000,
                      WiFi.status() == WL_CONNECTED ? "OK" : "FAIL",
                      ESP.getFreeHeap() / 1024);
        lastStatus = millis();
    }
    
    delay(100);
}