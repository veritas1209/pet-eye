// DS18B20 센서 진단 및 수정된 코드
// src/ds18b20_sensor.cpp - 개선된 버전

#include "ds18b20_sensor.h"
#include "config.h"
#include "sensor_data.h"

// 전역 센서 객체 정의
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// DS18B20 진단 함수
void diagnosticDS18B20() {
    Serial.println("\n=== DS18B20 진단 시작 ===");
    
    // 1. 핀 상태 확인
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    int pinState = digitalRead(ONE_WIRE_BUS);
    Serial.printf("GPIO%d 핀 상태: %s\n", ONE_WIRE_BUS, pinState ? "HIGH (정상)" : "LOW (문제!)");
    
    if (pinState == LOW) {
        Serial.println("⚠️  경고: 핀이 LOW 상태입니다. 풀업 저항(4.7kΩ)이 연결되어 있는지 확인하세요.");
    }
    
    // 2. OneWire 버스 리셋 테스트
    Serial.println("OneWire 버스 리셋 테스트...");
    bool resetOK = oneWire.reset();
    Serial.printf("리셋 결과: %s\n", resetOK ? "성공 (센서 존재)" : "실패 (센서 없음)");
    
    if (!resetOK) {
        Serial.println("❌ OneWire 리셋 실패 - 하드웨어 연결을 확인하세요!");
        return;
    }
    
    // 3. ROM 검색으로 센서 주소 찾기
    Serial.println("ROM 검색 시작...");
    uint8_t addr[8];
    int deviceCount = 0;
    
    oneWire.reset_search();
    while (oneWire.search(addr)) {
        deviceCount++;
        Serial.printf("디바이스 %d 발견: ", deviceCount);
        for (int i = 0; i < 8; i++) {
            Serial.printf("%02X", addr[i]);
            if (i < 7) Serial.print(":");
        }
        
        // CRC 검증
        if (OneWire::crc8(addr, 7) != addr[7]) {
            Serial.println(" - CRC 오류!");
        } else {
            Serial.println(" - CRC 정상");
            
            // 디바이스 타입 확인
            switch (addr[0]) {
                case 0x10: Serial.println("  → DS18S20 (구형)"); break;
                case 0x22: Serial.println("  → DS18B20"); break;
                case 0x28: Serial.println("  → DS18B20"); break;
                default: Serial.printf("  → 알 수 없는 디바이스 (0x%02X)\n", addr[0]); break;
            }
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("❌ ROM 검색에서 디바이스를 찾을 수 없습니다!");
        Serial.println("\n🔧 해결 방법:");
        Serial.println("1. 연결 확인:");
        Serial.printf("   - VDD → 3.3V (NOT 5V!)\n");
        Serial.printf("   - GND → GND\n");
        Serial.printf("   - DQ  → GPIO%d\n", ONE_WIRE_BUS);
        Serial.println("2. 4.7kΩ 풀업 저항을 DQ와 VDD 사이에 연결");
        Serial.println("3. 센서가 정품 DS18B20인지 확인");
        Serial.println("4. 케이블이 너무 길지 않은지 확인 (3m 이내)");
    } else {
        Serial.printf("✅ %d개의 디바이스가 발견되었습니다.\n", deviceCount);
    }
    
    Serial.println("=== DS18B20 진단 완료 ===\n");
}

bool initDS18B20() {
    Serial.println("DS18B20 센서 초기화 중...");
    
    // 진단 실행
    diagnosticDS18B20();
    
    // DallasTemperature 라이브러리 초기화
    ds18b20.begin();
    
    // 약간의 대기시간 추가
    delay(100);
    
    // 센서 개수 확인
    sensors.sensor_count = ds18b20.getDeviceCount();
    Serial.printf("DallasTemperature 라이브러리에서 발견된 센서 개수: %d\n", sensors.sensor_count);
    
    if (sensors.sensor_count > 0) {
        sensors.ds18b20_found = true;
        
        // 각 센서 정보 상세 출력
        for (int i = 0; i < sensors.sensor_count; i++) {
            DeviceAddress deviceAddress;
            if (ds18b20.getAddress(deviceAddress, i)) {
                Serial.printf("센서 %d 정보:\n", i);
                Serial.printf("  주소: ");
                for (uint8_t j = 0; j < 8; j++) {
                    Serial.printf("%02X", deviceAddress[j]);
                    if (j < 7) Serial.print(":");
                }
                Serial.println();
                
                // 센서 타입 확인
                if (deviceAddress[0] == 0x28 || deviceAddress[0] == 0x22) {
                    Serial.printf("  타입: DS18B20\n");
                } else if (deviceAddress[0] == 0x10) {
                    Serial.printf("  타입: DS18S20 (구형)\n");
                } else {
                    Serial.printf("  타입: 알 수 없음 (0x%02X)\n", deviceAddress[0]);
                }
                
                // 전력 모드 확인
                if (ds18b20.readPowerSupply(deviceAddress)) {
                    Serial.printf("  전력: 외부 전원\n");
                } else {
                    Serial.printf("  전력: 패러사이트 파워\n");
                }
            }
        }
        
        // 해상도 설정 (9-12비트)
        ds18b20.setResolution(12);  // 최고 해상도
        Serial.printf("해상도 설정: %d비트\n", ds18b20.getResolution());
        
        // 변환 시간 설정 (비블로킹 모드)
        ds18b20.setWaitForConversion(false);
        
        // 첫 번째 온도 읽기 테스트 (더 안정적인 방법)
        Serial.println("첫 온도 읽기 테스트...");
        ds18b20.requestTemperatures();
        
        // 변환 완료까지 대기 (해상도에 따른 시간)
        int conversionTime = ds18b20.millisToWaitForConversion(ds18b20.getResolution());
        Serial.printf("변환 대기 시간: %dms\n", conversionTime);
        delay(conversionTime + 50); // 여유시간 50ms 추가
        
        float test_temp = ds18b20.getTempCByIndex(0);
        
        if (test_temp != DEVICE_DISCONNECTED_C && test_temp > -55 && test_temp < 125) {
            Serial.printf("✅ 테스트 온도: %.3f°C\n", test_temp);
            sensors.temperature = test_temp;
            sensors.temp_valid = true;
            return true;
        } else {
            Serial.printf("❌ 테스트 온도 읽기 실패: %.3f°C\n", test_temp);
            Serial.println("센서가 발견되었지만 온도를 읽을 수 없습니다.");
            sensors.ds18b20_found = false;
            return false;
        }
    } else {
        Serial.println("❌ DS18B20 센서를 찾을 수 없습니다.");
        
        // 연결 가이드 출력
        Serial.println("\n🔧 연결 가이드:");
        Serial.println("┌─────────────────────────────────────┐");
        Serial.println("│          DS18B20 연결법             │");
        Serial.println("├─────────────────────────────────────┤");
        Serial.printf("│ VDD (빨강)  → 3.3V                 │\n");
        Serial.printf("│ GND (검정)  → GND                  │\n");
        Serial.printf("│ DQ  (노랑)  → GPIO%-2d              │\n", ONE_WIRE_BUS);
        Serial.println("│                                     │");
        Serial.println("│ 풀업 저항: 4.7kΩ                   │");
        Serial.printf("│ DQ (GPIO%-2d) ─── 4.7kΩ ─── 3.3V   │\n", ONE_WIRE_BUS);
        Serial.println("└─────────────────────────────────────┘");
        Serial.println("\n⚠️  중요사항:");
        Serial.println("- 반드시 3.3V 사용 (5V 금지!)");
        Serial.println("- 풀업 저항 필수 (4.7kΩ)");
        Serial.println("- 케이블 길이 3m 이내");
        Serial.println("- 정품 DS18B20 사용");
        
        sensors.ds18b20_found = false;
        return false;
    }
}

void printConnectionGuide() {
    Serial.println("\n🔧 연결 가이드:");
    Serial.println("┌─────────────────────────────────────┐");
    Serial.println("│          DS18B20 연결법             │");
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ VDD (빨강)  → 3.3V                 │\n");
    Serial.printf("│ GND (검정)  → GND                  │\n");
    Serial.printf("│ DQ  (노랑)  → GPIO%-2d              │\n", ONE_WIRE_BUS);
    Serial.println("│                                     │");
    Serial.println("│ 풀업 저항: 4.7kΩ                   │");
    Serial.printf("│ DQ (GPIO%-2d) ─── 4.7kΩ ─── 3.3V   │\n", ONE_WIRE_BUS);
    Serial.println("└─────────────────────────────────────┘");
    Serial.println("\n⚠️  중요사항:");
    Serial.println("- 반드시 3.3V 사용 (5V 금지!)");
    Serial.println("- 풀업 저항 필수 (4.7kΩ)");
    Serial.println("- 케이블 길이 3m 이내");
    Serial.println("- 정품 DS18B20 사용");
}

void readTemperature() {
    unsigned long now = millis();
    if (now - sensors.last_temp_read >= TEMP_READ_INTERVAL) {
        
        if (sensors.ds18b20_found) {
            // 비블로킹 온도 읽기
            static bool conversionStarted = false;
            static unsigned long conversionStartTime = 0;
            
            if (!conversionStarted) {
                // 온도 변환 시작
                ds18b20.requestTemperatures();
                conversionStarted = true;
                conversionStartTime = now;
                Serial.println("DS18B20 온도 변환 시작...");
                return;
            }
            
            // 변환 완료 시간 확인
            int conversionTime = ds18b20.millisToWaitForConversion(ds18b20.getResolution());
            if (now - conversionStartTime >= conversionTime) {
                float temp_c = ds18b20.getTempCByIndex(0);
                
                // 유효한 온도 범위 확인
                if (temp_c != DEVICE_DISCONNECTED_C && temp_c > -55 && temp_c < 125) {
                    sensors.temperature = temp_c;
                    sensors.temp_valid = true;
                    Serial.printf("✅ DS18B20 온도: %.3f°C\n", sensors.temperature);
                } else {
                    Serial.printf("❌ DS18B20 온도 읽기 실패: %.3f°C\n", temp_c);
                    sensors.temp_valid = false;
                    sensors.temperature = -999.0;
                    
                    // 센서 재초기화 시도
                    Serial.println("센서 재초기화 시도...");
                    sensors.ds18b20_found = false;
                    delay(1000);
                    initDS18B20();
                }
                
                conversionStarted = false;
                sensors.last_temp_read = now;
                sensors.data_ready = true;
            }
        } else {
            // 센서가 없으면 재초기화 시도 (30초마다)
            static unsigned long lastReinitAttempt = 0;
            if (now - lastReinitAttempt >= 30000) {
                Serial.println("DS18B20 센서 재검색 시도...");
                initDS18B20();
                lastReinitAttempt = now;
            }
            
            sensors.temp_valid = false;
            sensors.temperature = -999.0;
            sensors.last_temp_read = now;
        }
    }
}