// src/main.cpp - 완전한 단일 파일 솔루션
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

// ===== 설정 =====
const char* WIFI_SSID = "PRO";
const char* WIFI_PASSWORD = "propro123";  // 실제 비밀번호로 변경

// 핀 설정
#define I2C_SDA 17
#define I2C_SCL 18
#define TEMP_SENSOR_PIN 4

// 웹서버
WebServer server(80);

// 센서 데이터 구조체
struct {
    float temperature;
    int i2cDeviceCount;
    unsigned long lastTempUpdate;
    unsigned long lastI2CUpdate;
} sensorData;

// ===== I2C 스캔 함수 =====
void scanI2CDevices() {
    Serial.println("I2C 스캔 중...");
    sensorData.i2cDeviceCount = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("✓ I2C 장치: 0x%02X\n", address);
            sensorData.i2cDeviceCount++;
        }
    }
    
    sensorData.lastI2CUpdate = millis();
    Serial.printf("총 %d개 I2C 장치 발견\n", sensorData.i2cDeviceCount);
}

// ===== 온도 센서 읽기 =====
void readTemperature() {
    int rawValue = analogRead(TEMP_SENSOR_PIN);
    float voltage = rawValue * (3.3 / 4095.0);
    
    // SEN050007 온도 센서 변환 (조정 필요)
    sensorData.temperature = (voltage - 0.5) * 100.0;
    sensorData.lastTempUpdate = millis();
    
    Serial.printf("온도: Raw=%d, V=%.3f, T=%.1f°C\n", 
                  rawValue, voltage, sensorData.temperature);
}

// ===== 웹페이지 핸들러 =====
void handleRoot() {
    Serial.println("🌐 웹 요청 받음!");
    
    // 실시간 데이터 업데이트
    readTemperature();
    
    String html = "<!DOCTYPE html>";
    html += "<html lang='ko'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32-S3 실시간 모니터링</title>";
    
    // CSS 스타일
    html += "<style>";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
    html += "body { font-family: 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 20px; }";
    html += ".container { max-width: 900px; margin: 0 auto; background: rgba(255,255,255,0.95); border-radius: 20px; padding: 30px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); }";
    html += ".header { text-align: center; margin-bottom: 40px; }";
    html += ".header h1 { color: #333; font-size: 2.5em; margin-bottom: 10px; }";
    html += ".header p { color: #666; font-size: 1.1em; }";
    html += ".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 30px; }";
    html += ".status-card { background: #d4edda; color: #155724; padding: 15px; border-radius: 10px; text-align: center; border-left: 5px solid #28a745; }";
    html += ".info-section { background: #f8f9fa; padding: 25px; border-radius: 15px; margin-bottom: 25px; }";
    html += ".info-section h2 { color: #495057; margin-bottom: 20px; border-bottom: 3px solid #007bff; padding-bottom: 10px; }";
    html += ".info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; }";
    html += ".info-item { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
    html += ".info-label { font-weight: bold; color: #6c757d; font-size: 0.9em; }";
    html += ".info-value { color: #495057; font-size: 1.1em; margin-top: 5px; }";
    html += ".sensor-section { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); color: white; padding: 25px; border-radius: 15px; margin-bottom: 25px; }";
    html += ".sensor-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px; }";
    html += ".sensor-card { background: rgba(255,255,255,0.2); padding: 20px; border-radius: 12px; text-align: center; backdrop-filter: blur(10px); }";
    html += ".sensor-value { font-size: 2.5em; font-weight: bold; margin: 10px 0; text-shadow: 0 2px 4px rgba(0,0,0,0.3); }";
    html += ".sensor-label { font-size: 1.1em; opacity: 0.9; }";
    html += ".control-section { text-align: center; }";
    html += ".btn { display: inline-block; padding: 12px 25px; margin: 8px; background: #007bff; color: white; text-decoration: none; border-radius: 25px; font-weight: bold; transition: all 0.3s; border: none; cursor: pointer; font-size: 16px; }";
    html += ".btn:hover { background: #0056b3; transform: translateY(-2px); box-shadow: 0 5px 15px rgba(0,123,255,0.4); }";
    html += ".btn-success { background: #28a745; }";
    html += ".btn-success:hover { background: #1e7e34; box-shadow: 0 5px 15px rgba(40,167,69,0.4); }";
    html += ".btn-info { background: #17a2b8; }";
    html += ".btn-info:hover { background: #138496; box-shadow: 0 5px 15px rgba(23,162,184,0.4); }";
    html += ".live-indicator { display: inline-block; width: 10px; height: 10px; background: #28a745; border-radius: 50%; margin-right: 8px; animation: pulse 2s infinite; }";
    html += "@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }";
    html += "@media (max-width: 768px) { .container { padding: 20px; } .header h1 { font-size: 2em; } .sensor-value { font-size: 2em; } }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    
    html += "<div class='container'>";
    
    // 헤더
    html += "<div class='header'>";
    html += "<h1>🚀 ESP32-S3 IoT Dashboard</h1>";
    html += "<p><span class='live-indicator'></span>실시간 센서 모니터링 시스템</p>";
    html += "</div>";
    
    // 상태 카드들
    html += "<div class='status-grid'>";
    html += "<div class='status-card'>✅ ESP32-S3 정상</div>";
    html += "<div class='status-card'>🌐 WiFi 연결됨</div>";
    html += "<div class='status-card'>🔥 웹서버 동작</div>";
    html += "<div class='status-card'>📊 센서 활성</div>";
    html += "</div>";
    
    // 시스템 정보
    html += "<div class='info-section'>";
    html += "<h2>💻 시스템 정보</h2>";
    html += "<div class='info-grid'>";
    html += "<div class='info-item'><div class='info-label'>칩 모델</div><div class='info-value'>" + String(ESP.getChipModel()) + " Rev " + String(ESP.getChipRevision()) + "</div></div>";
    html += "<div class='info-item'><div class='info-label'>CPU 클럭</div><div class='info-value'>" + String(ESP.getCpuFreqMHz()) + " MHz</div></div>";
    html += "<div class='info-item'><div class='info-label'>플래시 메모리</div><div class='info-value'>" + String(ESP.getFlashChipSize()/1024/1024) + " MB</div></div>";
    html += "<div class='info-item'><div class='info-label'>가용 힙</div><div class='info-value'>" + String(ESP.getFreeHeap()/1024) + " KB</div></div>";
    html += "<div class='info-item'><div class='info-label'>PSRAM</div><div class='info-value'>" + String(psramFound() ? "사용 가능" : "없음") + "</div></div>";
    html += "<div class='info-item'><div class='info-label'>가동 시간</div><div class='info-value'>" + String(millis()/1000) + " 초</div></div>";
    html += "</div>";
    html += "</div>";
    
    // 네트워크 정보
    html += "<div class='info-section'>";
    html += "<h2>🌐 네트워크 정보</h2>";
    html += "<div class='info-grid'>";
    html += "<div class='info-item'><div class='info-label'>WiFi SSID</div><div class='info-value'>" + WiFi.SSID() + "</div></div>";
    html += "<div class='info-item'><div class='info-label'>IP 주소</div><div class='info-value'>" + WiFi.localIP().toString() + "</div></div>";
    html += "<div class='info-item'><div class='info-label'>신호 강도</div><div class='info-value'>" + String(WiFi.RSSI()) + " dBm</div></div>";
    html += "<div class='info-item'><div class='info-label'>게이트웨이</div><div class='info-value'>" + WiFi.gatewayIP().toString() + "</div></div>";
    html += "</div>";
    html += "</div>";
    
    // 센서 데이터
    html += "<div class='sensor-section'>";
    html += "<h2>📊 실시간 센서 데이터</h2>";
    html += "<div class='sensor-grid'>";
    html += "<div class='sensor-card'>";
    html += "<div class='sensor-label'>🌡️ 온도</div>";
    html += "<div class='sensor-value' id='temperature'>" + String(sensorData.temperature, 1) + "°C</div>";
    html += "<div style='font-size:0.9em; opacity:0.8;'>Raw: " + String(analogRead(TEMP_SENSOR_PIN)) + "</div>";
    html += "</div>";
    html += "<div class='sensor-card'>";
    html += "<div class='sensor-label'>🔌 I2C 장치</div>";
    html += "<div class='sensor-value' id='i2cCount'>" + String(sensorData.i2cDeviceCount) + "</div>";
    html += "<div style='font-size:0.9em; opacity:0.8;'>개 발견됨</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // 컨트롤 버튼
    html += "<div class='control-section'>";
    html += "<h2 style='color: #495057; margin-bottom: 20px;'>🎮 제어판</h2>";
    html += "<button class='btn' onclick='location.reload()'>🔄 전체 새로고침</button>";
    html += "<button class='btn btn-success' onclick='updateSensors()'>📊 센서 갱신</button>";
    html += "<button class='btn btn-info' onclick='window.open(\"/api\", \"_blank\")'>🔗 API 데이터</button>";
    html += "<button class='btn btn-info' onclick='toggleAutoUpdate()'>⏱️ 자동 갱신 토글</button>";
    html += "</div>";
    
    // JavaScript
    html += "<script>";
    html += "let autoUpdate = true;";
    html += "let updateInterval;";
    
    html += "function updateSensors() {";
    html += "  fetch('/api')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';";
    html += "      document.getElementById('i2cCount').textContent = data.i2cDevices;";
    html += "      console.log('센서 데이터 업데이트됨:', data);";
    html += "    })";
    html += "    .catch(error => {";
    html += "      console.error('업데이트 오류:', error);";
    html += "      alert('센서 데이터 업데이트 실패: ' + error);";
    html += "    });";
    html += "}";
    
    html += "function toggleAutoUpdate() {";
    html += "  autoUpdate = !autoUpdate;";
    html += "  if (autoUpdate) {";
    html += "    updateInterval = setInterval(updateSensors, 3000);";
    html += "    alert('자동 갱신 활성화 (3초마다)');";
    html += "  } else {";
    html += "    clearInterval(updateInterval);";
    html += "    alert('자동 갱신 비활성화');";
    html += "  }";
    html += "}";
    
    html += "// 페이지 로드 시 자동 갱신 시작";
    html += "updateInterval = setInterval(updateSensors, 3000);";
    html += "console.log('ESP32-S3 대시보드 로드 완료');";
    html += "</script>";
    
    html += "</div>";
    html += "</body>";
    html += "</html>";
    
    server.send(200, "text/html", html);
    Serial.println("✅ 웹페이지 전송 완료!");
}

// ===== API 핸들러 =====
void handleAPI() {
    Serial.println("📊 API 요청 받음!");
    
    // 실시간 센서 데이터 업데이트
    readTemperature();
    
    String json = "{";
    json += "\"temperature\":" + String(sensorData.temperature, 2) + ",";
    json += "\"i2cDevices\":" + String(sensorData.i2cDeviceCount) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"totalHeap\":" + String(ESP.getHeapSize()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
    Serial.println("✅ API 응답 완료!");
}

// ===== 404 핸들러 =====
void handleNotFound() {
    Serial.printf("❌ 404 요청: %s\n", server.uri().c_str());
    
    String message = "🚫 404 - 페이지를 찾을 수 없습니다\n\n";
    message += "요청된 URI: " + server.uri() + "\n";
    message += "HTTP 메소드: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n\n";
    message += "📍 사용 가능한 엔드포인트:\n";
    message += "/ - 메인 대시보드\n";
    message += "/api - JSON API 데이터\n";
    
    server.send(404, "text/plain", message);
    Serial.println("✅ 404 응답 전송 완료");
}

// ===== 시스템 초기화 =====
void setup() {
    Serial.begin(115200);
    delay(3000);
    
    Serial.println("\n==========================================");
    Serial.println("    🚀 ESP32-S3 IoT 웹서버 시스템");
    Serial.println("==========================================");
    
    // 시스템 정보 출력
    Serial.printf("✓ Chip: %s (Rev %d)\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("✓ CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("✓ Flash: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
    Serial.printf("✓ Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("✓ PSRAM: %s\n", psramFound() ? "Available" : "Not found");
    
    if (psramFound()) {
        Serial.printf("✓ PSRAM Size: %d KB\n", ESP.getPsramSize() / 1024);
        Serial.printf("✓ Free PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
    }
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("✓ I2C 초기화 완료 (SDA:%d, SCL:%d)\n", I2C_SDA, I2C_SCL);
    
    // 온도 센서 핀 설정
    pinMode(TEMP_SENSOR_PIN, INPUT);
    Serial.printf("✓ 온도 센서 핀 설정 완료 (PIN:%d)\n", TEMP_SENSOR_PIN);
    
    // 초기 센서 데이터 읽기
    Serial.println("\n--- 초기 센서 스캔 ---");
    scanI2CDevices();
    readTemperature();
    
    // WiFi 연결
    Serial.println("\n--- WiFi 연결 시작 ---");
    Serial.printf("SSID: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("연결 중");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d/30]", attempts);
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi 연결 성공!");
        Serial.printf("✓ SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("✓ IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("✓ 신호 강도: %d dBm\n", WiFi.RSSI());
        Serial.printf("✓ 게이트웨이: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("✓ DNS: %s\n", WiFi.dnsIP().toString().c_str());
        
        // 웹서버 설정 및 시작
        Serial.println("\n--- 웹서버 시작 ---");
        server.on("/", handleRoot);
        server.on("/api", handleAPI);
        server.onNotFound(handleNotFound);
        server.begin();
        
        Serial.println("✅ 웹서버 시작 완료!");
        Serial.println("==========================================");
        Serial.printf("🌐 브라우저에서 접속: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📊 API 엔드포인트: http://%s/api\n", WiFi.localIP().toString().c_str());
        Serial.println("==========================================");
        
    } else {
        Serial.println("❌ WiFi 연결 실패!");
        Serial.println("SSID와 비밀번호를 확인하세요.");
        Serial.printf("WiFi 상태 코드: %d\n", WiFi.status());
    }
    
    Serial.println("\n🎉 시스템 초기화 완료!\n");
}

// ===== 메인 루프 =====
void loop() {
    // WiFi 연결 상태 관리
    if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
    } else {
        // WiFi 재연결 시도
        static unsigned long lastReconnectAttempt = 0;
        if (millis() - lastReconnectAttempt > 30000) { // 30초마다
            Serial.println("WiFi 재연결 시도...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            lastReconnectAttempt = millis();
        }
    }
    
    // 센서 데이터 업데이트 (2초마다)
    static unsigned long lastTempRead = 0;
    if (millis() - lastTempRead > 2000) {
        readTemperature();
        lastTempRead = millis();
    }
    
    // I2C 스캔 (10초마다)
    static unsigned long lastI2CScan = 0;
    if (millis() - lastI2CScan > 10000) {
        scanI2CDevices();
        lastI2CScan = millis();
    }
    
    // 상태 출력 (5초마다)
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 5000) {
        Serial.printf("[%lu] WiFi: %s | Heap: %d KB | Temp: %.1f°C | I2C: %d개 | 웹서버: 대기중\n",
                      millis() / 1000,
                      WiFi.status() == WL_CONNECTED ? "연결됨" : "끊어짐",
                      ESP.getFreeHeap() / 1024,
                      sensorData.temperature,
                      sensorData.i2cDeviceCount);
        lastStatusPrint = millis();
    }
    
    delay(100); // CPU 부하 방지
}