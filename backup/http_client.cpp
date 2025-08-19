// src/http_client.cpp - HTTP 통신 관리 구현 (중복 해결)
#include "http_client.h"
#include "config.h"
#include "sensor_data.h"
#include "wifi_manager.h"
#include "wifi_config.h"
#include <WiFi.h>

// 전역 HTTP 클라이언트 정의 (static으로 중복 방지)
static HTTPClient httpClient;

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
    
    // 동적 서버 URL 사용
    String serverUrl = wifi_config.is_configured ? wifi_config.server_url : SERVER_URL;
    
    httpClient.begin(serverUrl);
    httpClient.addHeader("Content-Type", "application/json");
    httpClient.addHeader("User-Agent", "ESP32-DS18B20");
    httpClient.setTimeout(HTTP_TIMEOUT);
    
    Serial.printf("서버로 전송 중: %s\n", serverUrl.c_str());
    
    int httpResponseCode = httpClient.POST(jsonData);
    
    bool success = false;
    if (httpResponseCode > 0) {
        String response = httpClient.getString();
        Serial.printf("서버 응답 코드: %d\n", httpResponseCode);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("✅ 데이터 전송 성공!");
            success = true;
        } else {
            Serial.printf("❌ 서버 오류: %d\n", httpResponseCode);
        }
    } else {
        Serial.printf("❌ HTTP 전송 실패: %s\n", httpClient.errorToString(httpResponseCode).c_str());
    }
    
    httpClient.end();
    return success;
}

bool sendCameraImage(camera_fb_t* fb) {
    if (!isWiFiConnected()) {
        Serial.println("WiFi 연결되지 않음 - 이미지 전송 건너뜀");
        return false;
    }
    
    if (!fb || fb->len == 0) {
        Serial.println("❌ 유효하지 않은 이미지 데이터");
        return false;
    }
    
    // 이미지 업로드용 URL
    String serverUrl = wifi_config.is_configured ? wifi_config.server_url : SERVER_URL;
    String imageUrl = serverUrl;
    imageUrl.replace("/api/sensors", "/api/pet-video");
    
    httpClient.begin(imageUrl);
    httpClient.addHeader("Content-Type", "image/jpeg");
    httpClient.addHeader("User-Agent", "LilyGO-T-Camera-S3");
    httpClient.addHeader("X-Device-ID", device_info.device_id);
    httpClient.addHeader("X-Device-Type", "LilyGO-T-Camera-S3");
    httpClient.addHeader("X-Image-Size", String(fb->len));
    httpClient.addHeader("X-Image-Width", String(fb->width));
    httpClient.addHeader("X-Image-Height", String(fb->height));
    httpClient.addHeader("X-Project", "PetEye");
    httpClient.setTimeout(HTTP_TIMEOUT * 2);
    
    Serial.printf("📸 LilyGO 이미지 전송: %s (%d bytes)\n", imageUrl.c_str(), fb->len);
    
    int httpResponseCode = httpClient.POST(fb->buf, fb->len);
    
    bool success = false;
    if (httpResponseCode > 0) {
        String response = httpClient.getString();
        Serial.printf("서버 응답 코드: %d\n", httpResponseCode);
        
        if (httpResponseCode == 200 || httpResponseCode == 201) {
            Serial.println("✅ LilyGO 이미지 전송 성공!");
            success = true;
        } else {
            Serial.printf("❌ 서버 오류: %d\n", httpResponseCode);
        }
    } else {
        Serial.printf("❌ 이미지 전송 실패: %s\n", httpClient.errorToString(httpResponseCode).c_str());
    }
    
    httpClient.end();
    return success;
}

bool sendImageAndSensorData(camera_fb_t* fb) {
    // 이미지와 센서 데이터를 함께 전송
    bool imageSuccess = sendCameraImage(fb);
    delay(100); // 서버 부하 방지
    bool sensorSuccess = sendSensorData();
    
    return imageSuccess && sensorSuccess;
}