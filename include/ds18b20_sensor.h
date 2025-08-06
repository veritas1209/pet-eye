// include/ds18b20_sensor.h - DS18B20 온도센서 관리
#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 전역 센서 객체
extern OneWire oneWire;
extern DallasTemperature ds18b20;

// DS18B20 관리 함수들
bool initDS18B20();
void readTemperature();

#endif