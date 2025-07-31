#include "web_server.h"
#include "wifi_manager.h"
#include "sensor_manager.h"

WebServerManager webServerManager;

WebServerManager::WebServerManager() : server(WEB_SERVER_PORT) {}

bool WebServerManager::init() {
    if (!wifiManager.isWiFiConnected()) {
        DEBUG_PRINTLN("WiFi 연결되지 않음 - 웹서버 시작 불가");
        return false;
    }
    
    DEBUG_PRINTLN("\n--- 웹서버 초기화 시작 ---");
    setupRoutes();
    server.begin();
    
    DEBUG_PRINTLN("✓ 웹서버 시작 완료!");
    DEBUG_PRINTF("🌐 브라우저 접속: http://%s\n", WiFi.localIP().toString().c_str());
    
    return true;
}

void WebServerManager::setupRoutes() {
    server.on("/", [this](){ this->handleRoot(); });
    server.on("/api/status", [this](){ this->handleAPI(); });
    server.on("/api/sensors", [this](){ this->handleSensors(); });
    server.onNotFound([this](){ this->handleNotFound(); });
}

void WebServerManager::handleClient() {
    if (wifiManager.isWiFiConnected()) {
        server.handleClient();
    }
}

void WebServerManager::handleRoot() {
    server.send(200, "text/html", generateMainHTML());
}

void WebServerManager::handleAPI() {
    StaticJsonDocument<500> doc;
    
    // 시스템 정보
    doc["system"]["chip"] = ESP.getChipModel();
    doc["system"]["revision"] = ESP.getChipRevision();
    doc["system"]["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["system"]["flashSize"] = ESP.getFlashChipSize();
    doc["system"]["freeHeap"] = ESP.getFreeHeap();
    doc["system"]["psramFound"] = psramFound();
    
    if (psramFound()) {
        doc["system"]["psramSize"] = ESP.getPsramSize();
        doc["system"]["freePsram"] = ESP.getFreePsram();
    }
    
    // 네트워크 정보
    doc["network"]["connected"] = wifiManager.isWiFiConnected();
    if (wifiManager.isWiFiConnected()) {
        doc["network"]["ip"] = WiFi.localIP().toString();
        doc["network"]["rssi"] = WiFi.RSSI();
        doc["network"]["ssid"] = WiFi.SSID();
    }
    
    // 센서 정보
    doc["sensors"]["temperature"] = sensorData.temperature;
    doc["sensors"]["tempValid"] = sensorData.tempValid;
    doc["sensors"]["i2cDevices"] = sensorData.i2cDeviceCount;
    doc["sensors"]["lastUpdate"] = sensorData.lastUpdate;
    
    // 상태 정보
    doc["status"]["step"] = 4;
    doc["status"]["uptime"] = millis();
    doc["status"]["timestamp"] = millis();
    doc["status"]["success"] = true;
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void WebServerManager::handleSensors() {
    server.send(200, "application/json", sensorManager.getSensorJSON());
}

void WebServerManager::handleNotFound() {
    String message = "404 - 페이지를 찾을 수 없습니다\n\n";
    message += "URI: " + server.uri() + "\n";
    message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
    server.send(404, "text/plain", message);
}

String WebServerManager::generateMainHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>T-Camera S3 Step 4</title>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
    html += ".container{background:white;padding:20px;border-radius:10px;max-width:800px;margin:0 auto;box-shadow:0 2px 10px rgba(0,0,0,0.1)}";
    html += ".status{padding:15px;margin:10px 0;border-radius:5px}";
    html += ".success{background:#d4edda;color:#155724;border:1px solid #c3e6cb}";
    html += ".info{background:#e2e3e5;color:#383d41;padding:10px;margin:5px 0;border-radius:3px}";
    html += ".sensor{background:#f8f9fa;padding:15px;margin:10px 0;border-radius:5px;border-left:4px solid #007bff}";
    html += ".value{font-size:1.5em;color:#007bff;font-weight:bold}";
    html += "button{padding:12px 20px;background:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;margin:5px;font-size:14px}";
    html += "button:hover{background:#0056b3}";
    html += ".api-btn{background:#28a745}.api-btn:hover{background:#1e7e34}";
    html += "h1{color:#333;text-align:center}h2{color:#555;border-bottom:2px solid #007bff;padding-bottom:10px}";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<h1>🌡️ T-Camera S3 Step 4: 센서 테스트</h1>";
    
    // 상태 표시
    html += "<div class='status success'>✓ ESP32-S3 정상 동작</div>";
    html += "<div class='status success'>✓ WiFi 연결 성공</div>";
    html += "<div class='status success'>✓ 웹서버 정상 동작</div>";
    html += "<div class='status success'>✓ 센서 시스템 정상 동작</div>";
    
    // 시스템 정보
    html += generateSystemInfoHTML();
    
    // 네트워크 정보
    html += generateNetworkInfoHTML();
    
    // 센서 정보
    html += "<h2>📊 센서 데이터</h2>";
    html += "<div class='sensor'>";
    html += "<h3>🌡️ 온도 센서</h3>";
    html += "<div class='value' id='temp'>" + String(sensorData.temperature, 2) + "°C</div>";
    html += "</div>";
    html += "<div class='sensor'>";
    html += "<h3>🔌 I2C 장치</h3>";
    html += "<div class='value'>" + String(sensorData.i2cDeviceCount) + " 개 발견</div>";
    html += "</div>";
    
    // 컨트롤 버튼
    html += "<h2>🔧 제어</h2>";
    html += "<button onclick='location.reload()'>🔄 새로고침</button>";
    html += "<button class='api-btn' onclick='updateSensors()'>📊 센서 업데이트</button>";
    html += "<button onclick='window.open(\"/api/status\",\"_blank\")'>🔗 전체 API 보기</button>";
    html += "<button onclick='window.open(\"/api/sensors\",\"_blank\")'>📈 센서 API 보기</button>";
    
    // JavaScript
    html += "<script>";
    html += "function updateSensors(){";
    html += "fetch('/api/sensors').then(r=>r.json()).then(d=>{";
    html += "document.getElementById('temp').innerHTML=d.temperature.toFixed(2)+'°C';";
    html += "});";
    html += "}";
    html += "setInterval(updateSensors,3000);"; // 3초마다 자동 업데이트
    html += "</script>";
    
    html += "</div></body></html>";
    return html;
}

String WebServerManager::generateSystemInfoHTML() {
    String html = "<h2>💻 시스템 정보</h2>";
    html += "<div class='info'><strong>Chip:</strong> " + String(ESP.getChipModel()) + " (Rev " + String(ESP.getChipRevision()) + ")</div>";
    html += "<div class='info'><strong>CPU:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</div>";
    html += "<div class='info'><strong>Flash:</strong> " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB</div>";
    html += "<div class='info'><strong>Free Heap:</strong> " + String(ESP.getFreeHeap() / 1024) + " KB</div>";
    html += "<div class='info'><strong>PSRAM:</strong> " + String(psramFound() ? "Available" : "Not found") + "</div>";
    
    if (psramFound()) {
        html += "<div class='info'><strong>PSRAM Size:</strong> " + String(ESP.getPsramSize() / 1024) + " KB</div>";
        html += "<div class='info'><strong>Free PSRAM:</strong> " + String(ESP.getFreePsram() / 1024) + " KB</div>";
    }
    
    html += "<div class='info'><strong>Uptime:</strong> " + String(millis() / 1000) + " seconds</div>";
    return html;
}

String WebServerManager::generateNetworkInfoHTML() {
    String html = "<h2>🌐 네트워크 정보</h2>";
    if (wifiManager.isWiFiConnected()) {
        html += "<div class='info'><strong>Status:</strong> Connected</div>";
        html += "<div class='info'><strong>SSID:</strong> " + WiFi.SSID() + "</div>";
        html += "<div class='info'><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</div>";
        html += "<div class='info'><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</div>";
        html += "<div class='info'><strong>Gateway:</strong> " + WiFi.gatewayIP().toString() + "</div>";
    } else {
        html += "<div class='info'><strong>Status:</strong> Disconnected</div>";
    }
    return html;
}