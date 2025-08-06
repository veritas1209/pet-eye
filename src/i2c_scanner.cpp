// src/i2c_scanner.cpp - I2C 디바이스 스캐너 구현
#include "i2c_scanner.h"
#include "config.h"
#include "sensor_data.h"

// I2C 스캔 변수들 정의
uint8_t current_i2c_addr = 0x08;
bool i2c_scan_complete = false;

void initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_FREQ);
    Serial.println("I2C 초기화 완료");
}

void scanI2C() {
    static unsigned long last_addr_check = 0;
    unsigned long now = millis();
    
    // 초기 스캔이 완료되지 않았거나, 5분마다 재스캔
    if (!i2c_scan_complete || (i2c_scan_complete && now - sensors.last_i2c_scan >= I2C_SCAN_INTERVAL)) {
        
        // 10ms마다 주소 하나씩 체크 (비블로킹)
        if (now - last_addr_check >= I2C_SCAN_DELAY) {
            Wire.setTimeout(I2C_TIMEOUT);
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