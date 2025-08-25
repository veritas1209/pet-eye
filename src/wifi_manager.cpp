#include "wifi_manager.h"
#include <ArduinoJson.h>

WiFiCredentials WiFiManager::credentials;
Preferences WiFiManager::preferences;

void WiFiManager::init() {
    loadCredentials();
    
    // WiFi 연결 시도
    if (credentials.valid) {
        Serial.println("Found saved WiFi credentials");
        if (!connect()) {
            // 연결 실패 시 자격증명 삭제
            clearCredentials();
            DebugSystem::log("Cleared invalid WiFi credentials");
            startAP();
        }
    } else {
        DebugSystem::log("No saved credentials, starting AP mode");
        startAP();
    }
    
    // mDNS 시작
    if (MDNS.begin(DEVICE_NAME)) {
        DebugSystem::log("mDNS started: http://" + String(DEVICE_NAME) + ".local");
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
    }
}

bool WiFiManager::connect() {
    if (!credentials.valid) {
        return false;
    }
    
    DebugSystem::log("Connecting to WiFi: " + String(credentials.ssid));
    
    // 기존 연결 종료
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_NAME);
    WiFi.begin(credentials.ssid, credentials.password);
    
    // 연결 대기 (20초)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.print(" [Status: ");
            Serial.print(WiFi.status());
            Serial.print("] ");
        }
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        sysStatus.wifiConnected = true;
        sysStatus.localIP = WiFi.localIP();
        DebugSystem::log("✅ WiFi connected!");
        DebugSystem::log("IP: " + WiFi.localIP().toString());
        DebugSystem::log("RSSI: " + String(WiFi.RSSI()) + " dBm");
        return true;
    } else {
        DebugSystem::log("❌ WiFi connection failed! Status: " + String(WiFi.status()));
        sysStatus.wifiConnected = false;
        return false;
    }
}

void WiFiManager::startAP() {
    DebugSystem::log("Starting Access Point mode");
    
    WiFi.mode(WIFI_AP);
    
    // MAC 주소로 고유 AP 이름 생성
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[32];
    sprintf(apName, "%s-%02X%02X", DEFAULT_AP_SSID, mac[4], mac[5]);
    
    WiFi.softAP(apName, DEFAULT_AP_PASS);
    sysStatus.localIP = WiFi.softAPIP();
    
    DebugSystem::log("AP Started: " + String(apName));
    DebugSystem::log("AP IP: " + WiFi.softAPIP().toString());
}

void WiFiManager::loadCredentials() {
    preferences.begin("peteye", false);
    preferences.getBytes("wifi", &credentials, sizeof(credentials));
    preferences.end();
    
    if (strlen(credentials.ssid) > 0) {
        credentials.valid = true;
        DebugSystem::log("WiFi credentials loaded from memory");
    } else {
        credentials.valid = false;
        DebugSystem::log("No stored WiFi credentials found");
    }
}

void WiFiManager::saveCredentials(const char* ssid, const char* password) {
    strcpy(credentials.ssid, ssid);
    strcpy(credentials.password, password);
    credentials.valid = true;
    
    preferences.begin("peteye", false);
    preferences.putBytes("wifi", &credentials, sizeof(credentials));
    preferences.end();
    
    DebugSystem::log("WiFi credentials saved: " + String(ssid));
}

void WiFiManager::clearCredentials() {
    memset(&credentials, 0, sizeof(credentials));
    credentials.valid = false;
    
    preferences.begin("peteye", false);
    preferences.clear();
    preferences.end();
    
    DebugSystem::log("WiFi credentials cleared");
}

bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::checkConnection() {
    if (credentials.valid && !isConnected()) {
        DebugSystem::log("WiFi disconnected, attempting reconnection...");
        connect();
    }
}

String WiFiManager::scanNetworks() {
    DebugSystem::log("Starting WiFi scan");
    int n = WiFi.scanNetworks();
    
    JsonDocument doc;  // ArduinoJson 7.x 문법
    JsonArray networks = doc["networks"].to<JsonArray>();
    
    for (int i = 0; i < n; i++) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    String response;
    serializeJson(doc, response);
    
    DebugSystem::log("WiFi scan complete: " + String(n) + " networks found");
    return response;
}