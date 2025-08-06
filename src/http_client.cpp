// src/http_client.cpp - HTTP 통신 관리 구현
#include "http_client.h"
#include "config.h"
#include "sensor_data.h"
#include "wifi_manager.h"
#include <WiFi.h>

// 전역 HTTP 클라이언트 정의
HTTPClient http;

String createSensorJSON() {
    String json = "{";
    
    // 디바이스 정보
    json += "\"device_id\":\"" + device_info.device_id + "\",";
    json += "\"device_name\":\"" + String(DEVICE_NAME) + "\",";
    json += "\"location\":\"" + String(DEVICE_LOCATION) + "\",";
    
    // 센서 데이터
    json += "\"timestamp\":" + String(millis()) + ",";
    
    if (sensors.temp_valid) {
        json += "\"temperature\":" + String(sensors.temperature, 2) + ",";
        json += "\"temperature_valid\":true,";
    } else {
        json += "\"temperature\":null,";
        json += "\"temperature_valid\":false,";
    }
    
    json += "\"sensor_type\":\"DS18B20\",";
    json += "\"sensor_count\":" + String(sensors.sensor_count) + ",";
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

bool sendSensorData() {
    if (!isWiFiConnected()) {
        Serial.println("WiFi 연결되지 않음 - 전송 건너뜀");
        return false;
    }
    
    String jsonData = createSensorJSON();
    
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-DS18B20");
    http.setTimeout(HTTP_TIMEOUT);
    
    Serial.printf("서버로 전송 중: %s\n", SERVER_URL);
    
    int httpResponseCode = http.POST(jsonData);
    
    bool success = false;
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("서버 응답 코드: %d\n", httpResponseCode);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("✅ 데이터 전송 성공!");
            success = true;
        } else {
            Serial.printf("❌ 서버 오류: %d\n", httpResponseCode);
        }
    } else {
        Serial.printf("❌ HTTP 전송 실패: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    return success;
}