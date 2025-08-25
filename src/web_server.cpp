#include "web_server.h"
#include "web_pages.h"
#include "wifi_manager.h"
#include "debug_system.h"
#include "sensor_manager.h"
#include "camera_manager.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <OneWire.h>  // 온도 센서 진단용 추가

WebServer WebServerManager::server(WEB_SERVER_PORT);

void WebServerManager::init() {
    // 메인 페이지
    server.on("/", HTTP_GET, handleRoot);
    server.on("/debug", HTTP_GET, handleDebugPage);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/stream", HTTP_GET, handleStream);
    
    // API 엔드포인트
    server.on("/api/debug", HTTP_GET, handleAPIDebug);
    server.on("/api/status", HTTP_GET, handleAPIStatus);
    server.on("/api/clear", HTTP_POST, handleAPIClear);
    server.on("/api/test/camera", HTTP_POST, handleAPITestCamera);
    server.on("/api/test/temperature", HTTP_POST, handleAPITestTemperature);
    server.on("/api/test/api", HTTP_POST, handleAPITestAPI);
    server.on("/api/reboot", HTTP_POST, handleAPIReboot);
    
    // Favicon 처리 (404 방지)
    server.on("/favicon.ico", HTTP_GET, []() {
        server.send(204);  // No content
    });
    
    server.onNotFound(handleNotFound);
    
    server.begin();
    DebugSystem::log("Web server started on port " + String(WEB_SERVER_PORT));
}

void WebServerManager::handle() {
    server.handleClient();
}

void WebServerManager::handleRoot() {
    String html = String(INDEX_HTML);
    
    // 플레이스홀더 교체
    html.replace("%DEVICE_ID%", sysStatus.deviceId);
    html.replace("%WIFI_STATUS%", sysStatus.wifiConnected ? "Connected" : "Not Connected");
    html.replace("%WIFI_CLASS%", sysStatus.wifiConnected ? "online" : "offline");
    html.replace("%IP_ADDRESS%", sysStatus.localIP.toString());
    html.replace("%CAM_STATUS%", sysStatus.cameraInitialized ? "Ready" : "Not Initialized");
    html.replace("%CAM_CLASS%", sysStatus.cameraInitialized ? "online" : "offline");
    html.replace("%TEMPERATURE%", String(sysStatus.currentTemp, 1) + "°C");
    
    server.send(200, "text/html", html);
}

void WebServerManager::handleDebugPage() {
    server.send(200, "text/html", DEBUG_HTML);
}

void WebServerManager::handleScan() {
    String response = WiFiManager::scanNetworks();
    server.send(200, "application/json", response);
}

void WebServerManager::handleSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    if (ssid.length() == 0) {
        server.send(400, "text/html", "<html><body><h2>Error: SSID cannot be empty</h2><a href='/'>Go Back</a></body></html>");
        return;
    }
    
    DebugSystem::log("Saving WiFi credentials: " + ssid);
    WiFiManager::saveCredentials(ssid.c_str(), password.c_str());
    
    String html = "<html><head><meta charset='utf-8'></head>";
    html += "<body style='font-family: Arial; text-align: center; padding: 50px;'>";
    html += "<h2>✅ Configuration Saved!</h2>";
    html += "<p>Device will restart and connect to: <strong>" + ssid + "</strong></p>";
    html += "<p>After restart:</p>";
    html += "<ol style='text-align: left; display: inline-block;'>";
    html += "<li>Connect your device to the same WiFi network</li>";
    html += "<li>Access PetEye at: <strong>http://peteye.local</strong></li>";
    html += "<li>Or check Serial Monitor for IP address</li>";
    html += "</ol>";
    html += "<p>Restarting in 5 seconds...</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}

void WebServerManager::handleStream() {
    if (!sysStatus.cameraInitialized) {
        server.send(503, "text/plain", "Camera not initialized");
        return;
    }
    
    String html = "<html><body style='text-align:center;'>";
    html += "<h1>PetEye Camera Stream</h1>";
    html += "<img src='http://" + sysStatus.localIP.toString() + ":81/stream' style='width:100%; max-width:640px;'/>";
    html += "<br><a href='/'>Back to Configuration</a>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void WebServerManager::handleAPIDebug() {
    server.send(200, "text/plain", DebugSystem::getLog());
}

void WebServerManager::handleAPIStatus() {
    JsonDocument doc;  // ArduinoJson 7.x 문법
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["rssi"] = WiFi.RSSI();
    doc["temperature"] = sysStatus.currentTemp;
    doc["wifiConnected"] = sysStatus.wifiConnected;
    doc["cameraReady"] = sysStatus.cameraInitialized;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebServerManager::handleAPIClear() {
    DebugSystem::clear();
    server.send(200, "text/plain", "OK");
}

void WebServerManager::handleAPITestCamera() {
    DebugSystem::log("Testing camera...");
    bool result = CameraManager::testCapture();
    server.send(200, "text/plain", result ? "OK" : "FAILED");
}

void WebServerManager::handleAPITestTemperature() {
    DebugSystem::log("=== Temperature Sensor Diagnostic Test ===");
    
    // 1. GPIO 핀 상태 체크
    DebugSystem::log("1. Checking GPIO" + String(TEMP_SENSOR_PIN) + " state...");
    pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);
    delay(10);
    int pinState = digitalRead(TEMP_SENSOR_PIN);
    DebugSystem::log("   Pin state (pullup): " + String(pinState ? "HIGH" : "LOW"));
    
    // 2. OneWire 버스 체크
    DebugSystem::log("2. Testing OneWire bus...");
    OneWire testWire(TEMP_SENSOR_PIN);
    uint8_t resetResult = testWire.reset();
    DebugSystem::log("   Bus reset: " + String(resetResult ? "SUCCESS" : "FAILED"));
    
    // 3. 장치 검색
    DebugSystem::log("3. Scanning for devices...");
    uint8_t address[8];
    int count = 0;
    testWire.reset_search();
    
    while (testWire.search(address)) {
        count++;
        String addr = "   Device " + String(count) + ": ";
        for (int i = 0; i < 8; i++) {
            if (address[i] < 16) addr += "0";
            addr += String(address[i], HEX);
            if (i < 7) addr += ":";
        }
        DebugSystem::log(addr);
        
        // CRC 체크
        if (OneWire::crc8(address, 7) == address[7]) {
            DebugSystem::log("   └─ CRC: OK, Type: 0x" + String(address[0], HEX));
        } else {
            DebugSystem::log("   └─ CRC: FAILED!");
        }
    }
    
    if (count == 0) {
        DebugSystem::log("   ❌ No devices found!");
        
        // 4. 추가 하드웨어 체크
        DebugSystem::log("4. Hardware connection check:");
        DebugSystem::log("   - Check 4.7kΩ pullup resistor between data and VCC");
        DebugSystem::log("   - Verify JST connector wiring:");
        DebugSystem::log("     • Red = VCC (3.3V)");
        DebugSystem::log("     • Yellow = Data (GPIO" + String(TEMP_SENSOR_PIN) + ")");
        DebugSystem::log("     • Black = GND");
        
        // 5. 핀 토글 테스트
        DebugSystem::log("5. Pin toggle test...");
        pinMode(TEMP_SENSOR_PIN, OUTPUT);
        int toggleCount = 0;
        for (int i = 0; i < 10; i++) {
            digitalWrite(TEMP_SENSOR_PIN, i % 2);
            delay(1);
            if (digitalRead(TEMP_SENSOR_PIN) == (i % 2)) {
                toggleCount++;
            }
        }
        pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);
        DebugSystem::log("   Toggle success rate: " + String(toggleCount * 10) + "%");
        
        server.send(200, "text/plain", "FAILED: No sensor found. Check debug log.");
    } else {
        // 센서를 찾은 경우 온도 읽기 시도
        DebugSystem::log("4. Attempting temperature read...");
        
        // DallasTemperature 라이브러리 재초기화
        DallasTemperature sensors(&testWire);
        sensors.begin();
        
        // 해상도 설정
        sensors.setResolution(12);
        
        // 온도 변환 요청
        DebugSystem::log("   Requesting temperature conversion...");
        sensors.requestTemperatures();
        
        // 충분한 변환 시간 대기
        DebugSystem::log("   Waiting 750ms for conversion...");
        delay(750);
        
        // 온도 읽기
        float temp = sensors.getTempCByIndex(0);
        DebugSystem::log("   Raw reading: " + String(temp, 2) + "°C");
        
        if (temp != DEVICE_DISCONNECTED_C && temp != 85.0) {
            sysStatus.currentTemp = temp;
            DebugSystem::log("   ✅ Temperature: " + String(temp, 2) + "°C");
            server.send(200, "text/plain", "OK: " + String(temp, 2) + "°C");
        } else if (temp == 85.0) {
            DebugSystem::log("   ⚠️ Got 85°C - Power-on reset value, retrying...");
            delay(100);
            sensors.requestTemperatures();
            delay(1000);
            temp = sensors.getTempCByIndex(0);
            DebugSystem::log("   Retry result: " + String(temp, 2) + "°C");
            
            if (temp != DEVICE_DISCONNECTED_C && temp != 85.0) {
                server.send(200, "text/plain", "OK after retry: " + String(temp, 2) + "°C");
            } else {
                server.send(200, "text/plain", "FAILED: Sensor power issue (85°C)");
            }
        } else {
            DebugSystem::log("   ❌ Read failed (got -127°C)");
            DebugSystem::log("   Possible causes:");
            DebugSystem::log("   - Insufficient conversion time");
            DebugSystem::log("   - Power supply issue");
            DebugSystem::log("   - Parasite power mode conflict");
            server.send(200, "text/plain", "FAILED: Sensor found but can't read (-127°C)");
        }
    }
    
    DebugSystem::log("=== End Diagnostic Test ===");
}

void WebServerManager::handleAPITestAPI() {
    DebugSystem::log("Testing API connection...");
    
    if (!sysStatus.wifiConnected) {
        DebugSystem::log("Cannot test API - WiFi not connected");
        server.send(200, "text/plain", "WiFi not connected");
        return;
    }
    
    HTTPClient http;
    http.begin(String(API_BASE_URL) + "/test");
    http.addHeader("Content-Type", "application/json");
    
    JsonDocument doc;  // ArduinoJson 7.x 문법
    doc["deviceId"] = sysStatus.deviceId;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
        DebugSystem::log("API test response code: " + String(httpCode));
        if (httpCode == HTTP_CODE_OK) {
            String response = http.getString();
            DebugSystem::log("API response: " + response);
        }
    } else {
        DebugSystem::log("API test failed: " + http.errorToString(httpCode));
    }
    
    http.end();
    server.send(200, "text/plain", "OK");
}

void WebServerManager::handleAPIReboot() {
    DebugSystem::log("System reboot requested");
    server.send(200, "text/plain", "Rebooting...");
    delay(1000);
    ESP.restart();
}

void WebServerManager::handleNotFound() {
    server.send(404, "text/plain", "404: Not Found");
}