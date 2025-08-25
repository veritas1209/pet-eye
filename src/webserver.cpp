#include "web_server.h"
#include "web_pages.h"
#include "wifi_manager.h"
#include "debug_system.h"
#include "sensor_manager.h"
#include "camera_manager.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

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
    DebugSystem::log("Testing temperature sensor...");
    float temp = SensorManager::readTemperature();
    if (temp != DEVICE_DISCONNECTED_C) {
        sysStatus.currentTemp = temp;
        DebugSystem::log("Temperature: " + String(temp, 2) + "°C");
        server.send(200, "text/plain", "OK: " + String(temp, 2) + "°C");
    } else {
        DebugSystem::log("Temperature sensor not found");
        server.send(200, "text/plain", "FAILED: Sensor not found");
    }
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