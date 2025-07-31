#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>

// 핀 정의
#define I2C_SDA 17
#define I2C_SCL 18
#define TEMP_SENSOR_PIN 4

// WiFi 설정
const char* ssid = "your_wifi_name";
const char* password = "your_wifi_password";

// 웹서버
WebServer server(80);

// 센서 데이터
struct {
    float temperature;
    bool tempValid;
    int i2cDevices;
} sensorData;

void scanI2C() {
    Serial.println("I2C 장치 스캔 중...");
    sensorData.i2cDevices = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("✓ I2C 장치 발견: 0x%02X\n", address);
            sensorData.i2cDevices++;
        }
    }
    
    Serial.printf("총 %d개의 I2C 장치 발견\n", sensorData.i2cDevices);
}

void readTemperature() {
    int rawValue = analogRead(TEMP_SENSOR_PIN);
    float voltage = rawValue * (3.3 / 4095.0);
    
    // SEN050007 온도 센서 변환 (데이터시트에 맞게 조정 필요)
    sensorData.temperature = (voltage - 0.5) * 100.0;
    sensorData.tempValid = true;
    
    Serial.printf("온도 센서 - Raw: %d, Voltage: %.3fV, Temp: %.2f°C\n", 
                  rawValue, voltage, sensorData.temperature);
}

void handleRoot() {
    String html = "<html><head><title>T-Camera S3</title></head><body>";
    html += "<h1>T-Camera S3 테스트</h1>";
    html += "<p>Chip: " + String(ESP.getChipModel()) + "</p>";
    html += "<p>Free Heap: " + String(ESP.getFreeHeap() / 1024) + " KB</p>";
    html += "<p>PSRAM: " + String(psramFound() ? "Available" : "Not found") + "</p>";
    html += "<p>WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "</p>";
    html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    html += "<br><a href='/' style='padding:10px; background:#007bff; color:white; text-decoration:none;'>새로고침</a>";
    html += " <a href='/api/status' style='padding:10px; background:#28a745; color:white; text-decoration:none;'>API 테스트</a>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleSensors() {
    StaticJsonDocument<300> doc;
    doc["temperature"] = sensorData.temperature;
    doc["tempValid"] = sensorData.tempValid;
    doc["i2cDevices"] = sensorData.i2cDevices;
    doc["step"] = 4;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    server.send(200, "application/json", jsonString);
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    
    Serial.println("\n=== T-Camera S3 Step 4: 센서 테스트 ===");
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("✓ I2C 초기화 완료");
    
    // 온도 센서 핀 설정
    pinMode(TEMP_SENSOR_PIN, INPUT);
    Serial.println("✓ 온도 센서 핀 설정 완료");
    
    // I2C 스캔
    scanI2C();
    
    // 초기 온도 읽기
    readTemperature();
    
    // WiFi 연결
    WiFi.begin(ssid, password);
    Serial.print("WiFi 연결 중");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\n✓ WiFi 연결 성공!");
    Serial.print("✓ IP 주소: ");
    Serial.println(WiFi.localIP());
    
    // 웹서버 설정
    server.on("/", handleRoot);
    server.on("/api/sensors", handleSensors);
    server.begin();
    
    Serial.println("✓ 웹서버 시작 완료!");
    Serial.print("🌐 브라우저에서 접속: http://");
    Serial.println(WiFi.localIP());
    Serial.println("=== Step 4 센서 테스트 완료 ===\n");
}

void loop() {
    server.handleClient();
    
    // 2초마다 센서 데이터 업데이트
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 2000) {
        readTemperature();
        lastSensorRead = millis();
    }
    
    // 10초마다 I2C 재스캔
    static unsigned long lastI2CScan = 0;
    if (millis() - lastI2CScan > 10000) {
        scanI2C();
        lastI2CScan = millis();
    }
    
    delay(100);
}