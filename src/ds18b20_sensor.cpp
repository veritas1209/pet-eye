// src/ds18b20_sensor.cpp - DS18B20 온도센서 관리 구현
#include "ds18b20_sensor.h"
#include "config.h"
#include "sensor_data.h"

// 전역 센서 객체 정의
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

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

void readTemperature() {
    unsigned long now = millis();
    if (now - sensors.last_temp_read >= TEMP_READ_INTERVAL) {
        
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