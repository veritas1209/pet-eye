#include "sensor_manager.h"

OneWire SensorManager::oneWire(TEMP_SENSOR_PIN);
DallasTemperature SensorManager::tempSensor(&oneWire);

void SensorManager::init() {
    if (ENABLE_TEMPERATURE) {
        DebugSystem::log("========== Temperature Sensor Debug ==========");
        DebugSystem::log("Initializing DS18B20 on GPIO " + String(TEMP_SENSOR_PIN));
        
        // GPIO 상태 체크
        pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);
        delay(10);
        int pinState = digitalRead(TEMP_SENSOR_PIN);
        DebugSystem::log("Pin initial state (with pullup): " + String(pinState ? "HIGH" : "LOW"));
        
        // OneWire 버스 리셋 테스트
        DebugSystem::log("Testing OneWire bus reset...");
        uint8_t resetResult = oneWire.reset();
        if (resetResult) {
            DebugSystem::log("✅ OneWire device detected (reset successful)");
        } else {
            DebugSystem::log("❌ No OneWire device found (reset failed)");
        }
        
        // OneWire 장치 검색
        DebugSystem::log("Searching for OneWire devices...");
        uint8_t address[8];
        int deviceCount = 0;
        
        oneWire.reset_search();
        delay(250);
        
        while (oneWire.search(address)) {
            deviceCount++;
            String addressStr = "Device " + String(deviceCount) + " found at: ";
            for (uint8_t i = 0; i < 8; i++) {
                if (address[i] < 16) addressStr += "0";
                addressStr += String(address[i], HEX);
                if (i < 7) addressStr += ":";
            }
            DebugSystem::log(addressStr);
            
            // ROM 코드 검증
            if (OneWire::crc8(address, 7) != address[7]) {
                DebugSystem::log("  ⚠️ CRC is not valid!");
            } else {
                DebugSystem::log("  ✅ CRC valid");
            }
            
            // 장치 타입 확인
            switch (address[0]) {
                case 0x10:
                    DebugSystem::log("  Type: DS18S20 or DS1820");
                    break;
                case 0x28:
                    DebugSystem::log("  Type: DS18B20");
                    break;
                case 0x22:
                    DebugSystem::log("  Type: DS1822");
                    break;
                default:
                    DebugSystem::log("  Type: Unknown (0x" + String(address[0], HEX) + ")");
            }
        }
        
        DebugSystem::log("Total OneWire devices found: " + String(deviceCount));
        
        // DallasTemperature 라이브러리 초기화
        tempSensor.begin();
        int dallasSensorCount = tempSensor.getDeviceCount();
        DebugSystem::log("DallasTemperature device count: " + String(dallasSensorCount));
        
        if (dallasSensorCount > 0) {
            sysStatus.tempSensorFound = true;
            
            // 각 센서의 해상도 설정 및 정보 출력
            for (int i = 0; i < dallasSensorCount; i++) {
                DeviceAddress deviceAddress;
                if (tempSensor.getAddress(deviceAddress, i)) {
                    tempSensor.setResolution(deviceAddress, 12);
                    int resolution = tempSensor.getResolution(deviceAddress);
                    DebugSystem::log("Sensor " + String(i) + " resolution set to: " + String(resolution) + " bits");
                    
                    // 파라사이트 전원 모드 체크
                    bool parasitic = tempSensor.isParasitePowerMode();
                    DebugSystem::log("Parasite power mode: " + String(parasitic ? "YES" : "NO"));
                }
            }
            
            // 첫 번째 온도 읽기 시도
            DebugSystem::log("Attempting first temperature reading...");
            tempSensor.requestTemperatures();
            delay(750); // 12비트 해상도에서 필요한 변환 시간
            
            float tempC = tempSensor.getTempCByIndex(0);
            if (tempC != DEVICE_DISCONNECTED_C) {
                DebugSystem::log("✅ First reading successful: " + String(tempC, 2) + "°C");
                sysStatus.currentTemp = tempC;
            } else {
                DebugSystem::log("❌ First reading failed (DEVICE_DISCONNECTED)");
            }
            
        } else {
            sysStatus.tempSensorFound = false;
            DebugSystem::log("❌ No DS18B20 temperature sensor found");
            
            // 추가 디버깅: 핀 토글 테스트
            DebugSystem::log("Performing pin toggle test...");
            pinMode(TEMP_SENSOR_PIN, OUTPUT);
            for (int i = 0; i < 5; i++) {
                digitalWrite(TEMP_SENSOR_PIN, LOW);
                delay(1);
                digitalWrite(TEMP_SENSOR_PIN, HIGH);
                delay(1);
            }
            pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);
            delay(10);
            
            // 전압 레벨 체크 (아날로그 읽기 가능한 경우)
            DebugSystem::log("Final pin state: " + String(digitalRead(TEMP_SENSOR_PIN) ? "HIGH" : "LOW"));
        }
        
        DebugSystem::log("========== End Temperature Sensor Debug ==========");
    }
    
    if (ENABLE_MPU6050) {
        // TODO: MPU6050 초기화
        DebugSystem::log("MPU6050 not yet implemented");
    }
}

void SensorManager::update() {
    // 온도 센서 업데이트
    if (ENABLE_TEMPERATURE && sysStatus.tempSensorFound) {
        if (millis() - sysStatus.lastTempRead > TEMP_READ_INTERVAL) {
            float temp = readTemperature();
            if (temp != DEVICE_DISCONNECTED_C) {
                sysStatus.currentTemp = temp;
                sysStatus.lastTempRead = millis();
                
                // 온도 변화가 1도 이상일 때만 로그
                static float lastLoggedTemp = 0;
                if (abs(temp - lastLoggedTemp) > 1.0) {
                    DebugSystem::log("Temperature: " + String(temp, 1) + "°C");
                    lastLoggedTemp = temp;
                }
            } else {
                // 읽기 실패 시 상세 로그
                static unsigned long lastErrorLog = 0;
                if (millis() - lastErrorLog > 10000) { // 10초마다 에러 로그
                    DebugSystem::log("⚠️ Temperature read failed - checking connection...");
                    
                    // 연결 재확인
                    uint8_t resetResult = oneWire.reset();
                    if (!resetResult) {
                        DebugSystem::log("❌ OneWire connection lost!");
                        sysStatus.tempSensorFound = false;
                    }
                    lastErrorLog = millis();
                }
            }
        }
    }
}

float SensorManager::readTemperature() {
    if (!sysStatus.tempSensorFound) {
        return DEVICE_DISCONNECTED_C;
    }
    
    // 온도 변환 요청
    tempSensor.requestTemperatures();
    
    // 충분한 변환 시간 대기 (12비트 = 750ms)
    delay(750);  // 중요! 이 지연이 필요합니다
    
    // 온도 읽기
    float temp = tempSensor.getTempCByIndex(0);
    
    // 디버깅: 비정상적인 값 체크
    if (temp == 85.0) {
        DebugSystem::log("⚠️ Got 85°C - possible power reset or connection issue");
        // 재시도
        delay(100);
        tempSensor.requestTemperatures();
        delay(750);
        temp = tempSensor.getTempCByIndex(0);
    } else if (temp == -127.0) {
        DebugSystem::log("⚠️ Got -127°C - retrying with longer delay...");
        // 더 긴 지연으로 재시도
        delay(100);
        tempSensor.requestTemperatures();
        delay(1000);  // 더 긴 대기
        temp = tempSensor.getTempCByIndex(0);
    }
    
    return temp;
}

bool SensorManager::isTemperatureSensorConnected() {
    return sysStatus.tempSensorFound;
}