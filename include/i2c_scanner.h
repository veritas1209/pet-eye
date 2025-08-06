// include/i2c_scanner.h - I2C 디바이스 스캐너
#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <Arduino.h>
#include <Wire.h>

// I2C 스캔 변수들
extern uint8_t current_i2c_addr;
extern bool i2c_scan_complete;

// I2C 관리 함수들
void initI2C();
void scanI2C();

#endif