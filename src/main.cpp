// src/main.cpp - DS18B20 디지털 온도센서용
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi 설정
const char* ssid = "PRO";
const char* password = "propro123";

// 서버 설정
const char* server_url = "http://192.168.0.27:3000/api/sensors"; // 서버 IP로 변경
const unsigned long SEND_INTERVAL = 10000; // 10초마다 전송

// 핀 설정
#define ONE_WIRE_BUS 46  // IO46에 DS18B20의 DQ 핀 연결
#define I2C_SDA 17
#define I2C_SCL 18

// DS18B20 설정
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// HTTP 클라이언트
HTTPClient http;

// 센서 데이터 구조체
struct SensorData {
    float temperature;
    bool temp_valid;
    int i2c_count;
    unsigned long last_temp_read;
    unsigned long last_i2c_scan;
    unsigned long last_send;
    bool data_ready;
    bool ds18b20_found;
    uint8_t sensor_count;
} sensors;

// 디바이스 정보
struct DeviceInfo {
    String device_id;
    String chip_model;
    int chip_revision;
    int cpu_freq;
    String mac_address;
} device_info;

// I2C 스캔 (비동기식)
uint8_t current_i2c_addr = 0x08;
bool i2c_scan_complete = false;

void scanI2C() {
    static unsigned long last_addr_check = 0;
    unsigned long now = millis();
    
    // 초기 스캔이 완료되지 않았거나, 5분마다 재스캔
    if (!i2c_scan_complete || (i2c_scan_complete && now - sensors.last_i2c_scan >= 300000)) {
        
        // 10ms마다 주소 하나씩 체크 (비블로킹)
        if (now - last_addr_check >= 10) {
            Wire.setTimeout(5);
            Wire.beginTransmission(current_i2c_addr);
            
            if (Wire.endTransmission() == 0) {
                if (!i2c_scan_complete) {
                    Serial.printf("I2C 발견: 0x%02X\n", current_i2c_addr);
                }
                sensors.i2c_count++;
            }
            
            current_i2c_addr++;
            last_addr_check = now;
            
            // 스캔 완료 체크
            if (current_i2c_addr >= 0x78 || sensors.i2c_count >= 10) {
                if (!i2c_scan_complete) {
                    Serial.printf("✅ I2C 스캔 완료: %d개 디바이스 발견\n", sensors.i2c_count);
                    i2c_scan_complete = true;
                }
                sensors.last_i2c_scan = now;
                current_i2c_addr = 0x08;
                sensors.data_ready = true;
            }
        }
    }
}

// DS18B20 온도 읽기 함수
void readTemperature() {
    unsigned long now = millis();
    if (now - sensors.last_temp_read >= 2000) { // 2초마다 읽기
        
        if (sensors.ds18b20_found) {
            Serial.println("DS18B20 온도 측정 시작...");
            ds18b20.requestTemperatures(); // 온도 변환 시작
            
            // 변환 완료 대기 (750ms 최대)
            delay(800); // 12비트 해상도: 750ms
            
            float temp_c = ds18b20.getTempCByIndex(0); // 첫 번째 센서 온도
            
            if (temp_c != DEVICE_DISCONNECTED_C) {
                sensors.temperature = temp_c;
                sensors.temp_valid = true;
                Serial.printf("✅ DS18B20 온도: %.2f°C\n", sensors.temperature);
            } else {
                Serial.println("❌ DS18B20 온도 읽기 실패!");
                sensors.temp_valid = false;
                sensors.temperature = -999.0;
            }
        } else {
            Serial.println("❌ DS18B20 센서를 찾을 수 없음!");
            sensors.temp_valid = false;
            sensors.temperature = -999.0;
        }
        
        sensors.last_temp_read = now;
        sensors.data_ready = true;
    }
}

// DS18B20 초기화 및 검색
bool initDS18B20() {
    Serial.println("DS18B20 센서 초기화 중...");
    
    ds18b20.begin();
    sensors.sensor_count = ds18b20.getDeviceCount();
    
    Serial.printf("발견된 DS18B20 센서 개수: %d\n", sensors.sensor_count);
    
    if (sensors.sensor_count > 0) {
        sensors.ds18b20_found = true;
        
        // 센서 정보 출력
        for (int i = 0; i < sensors.sensor_count; i++) {
            DeviceAddress deviceAddress;
            if (ds18b20.getAddress(deviceAddress, i)) {
                Serial.printf("센서 %d 주소: ", i);
                for (uint8_t j = 0; j < 8; j++) {
                    Serial.printf("%02X", deviceAddress[j]);
                }
                Serial.println();
            }
        }
        
        // 해상도 12비트로 설정 (최고 정밀도)
        ds18b20.setResolution(12);
        Serial.printf("해상도: %d비트\n", ds18b20.getResolution());
        
        // 첫 번째 온도 읽기 테스트
        ds18b20.requestTemperatures();
        delay(800);
        float test_temp = ds18b20.getTempCByIndex(0);
        
        if (test_temp != DEVICE_DISCONNECTED_C) {
            Serial.printf("✅ 테스트 온도: %.2f°C\n", test_temp);
            return true;
        } else {
            Serial.println("❌ 테스트 온도 읽기 실패");
            return false;
        }
    } else {
        Serial.println("❌ DS18B20 센서를 찾을 수 없습니다.");
        Serial.println("연결을 확인하세요:");
        Serial.printf("  VDD → 3.3V\n");
        Serial.printf("  GND → GND\n");
        Serial.printf("  DQ  → GPIO%d (4.7kΩ 풀업 저항 필요)\n", ONE_WIRE_BUS);
        sensors.ds18b20_found = false;
        return false;
    }
}

// JSON 데이터 생성
String createSensorJSON() {
    String json = "{";
    
    // 디바이스 정보
    json += "\"device_id\":\"" + device_info.device_id + "\",";
    json += "\"device_name\":\"T-Camera-S3-DS18B20\",";
    json += "\"location\":\"Lab-01\",";
    
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

// 서버에 데이터 전송
bool sendSensorData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi 연결되지 않음 - 전송 건너뜀");
        return false;
    }
    
    String jsonData = createSensorJSON();
    
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-DS18B20");
    http.setTimeout(5000);
    
    Serial.printf("서버로 전송 중: %s\n", server_url);
    
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

// WiFi 연결 함수
void connectWiFi() {
    Serial.printf("WiFi 연결 시도: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    
    WiFi.begin(ssid, password);
    Serial.print("연결 중");
    
    unsigned long startTime = millis();
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 30000) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        if (attempts % 10 == 0) {
            Serial.printf(" [%d]", attempts);
        }
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi 연결 성공!");
        Serial.printf("IP 주소: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("신호 강도: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("❌ WiFi 연결 실패!");
        Serial.printf("상태 코드: %d\n", WiFi.status());
    }
}

// WiFi 재연결 처리
void handleWiFiReconnection() {
    static unsigned long lastReconnectAttempt = 0;
    
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= 30000) {
            Serial.println("WiFi 재연결 시도...");
            WiFi.begin(ssid, password);
            lastReconnectAttempt = now;
        }
    }
}

// 초기화 함수
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== T-Camera S3 + DS18B20 센서 시작 ===");
    Serial.printf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap()/1024);
    
    // 디바이스 정보 설정
    device_info.device_id = WiFi.macAddress();
    device_info.chip_model = ESP.getChipModel();
    device_info.chip_revision = ESP.getChipRevision();
    device_info.cpu_freq = ESP.getCpuFreqMHz();
    device_info.mac_address = WiFi.macAddress();
    
    Serial.printf("디바이스 ID: %s\n", device_info.device_id.c_str());
    
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    Serial.println("I2C 초기화 완료");
    
    // DS18B20 초기화
    bool ds18b20_ok = initDS18B20();
    
    // 초기 센서 값 설정
    sensors.data_ready = false;
    sensors.last_send = 0;
    sensors.temp_valid = false;
    sensors.i2c_count = 0;
    
    if (ds18b20_ok) {
        readTemperature(); // 첫 온도 읽기
        Serial.printf("초기 온도: %.2f°C\n", sensors.temperature);
    }
    
    Serial.println("I2C 스캔은 백그라운드에서 진행됩니다...");
    
    // WiFi 연결
    connectWiFi();
    
    Serial.printf("서버 URL: %s\n", server_url);
    Serial.printf("전송 간격: %d초\n", SEND_INTERVAL / 1000);
    Serial.println("=== 초기화 완료 ===\n");
}

// 메인 루프
void loop() {
    // WiFi 연결 상태 확인 및 재연결
    handleWiFiReconnection();
    
    // 센서 데이터 읽기
    readTemperature();
    scanI2C();
    
    // 서버에 데이터 전송 (주기적)
    unsigned long now = millis();
    if (sensors.data_ready && (now - sensors.last_send >= SEND_INTERVAL)) {
        if (sendSensorData()) {
            sensors.data_ready = false;
        }
        sensors.last_send = now;
    }
    
    // 상태 출력 (5초마다)
    static unsigned long lastStatus = 0;
    if (now - lastStatus >= 5000) {
        String temp_status = sensors.temp_valid ? 
                           String(sensors.temperature, 2) + "°C" : "오류";
        String i2c_status = i2c_scan_complete ? String(sensors.i2c_count) : "스캔중";
        
        Serial.printf("[%lu] WiFi: %s | DS18B20: %s | I2C: %s | MEM: %dKB | 다음전송: %ds\n",
                      now / 1000,
                      WiFi.status() == WL_CONNECTED ? "연결됨" : "연결안됨",
                      temp_status.c_str(),
                      i2c_status.c_str(),
                      ESP.getFreeHeap() / 1024,
                      (SEND_INTERVAL - (now - sensors.last_send)) / 1000);
        lastStatus = now;
    }
    
    delay(100);
}