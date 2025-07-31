#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include "common.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class SensorModule {
private:
    Adafruit_MPU6050 mpu;
    bool mpuInitialized;
    unsigned long lastUpdate;

public:
    SensorModule();
    bool init();
    bool readTemperature(float& temperature);
    bool readMPU6050(float& accelX, float& accelY, float& accelZ, 
                     float& gyroX, float& gyroY, float& gyroZ);
    void updateSensorData();
    bool isMPUInitialized() const { return mpuInitialized; }
    void calibrateMPU();
};

extern SensorModule sensors;

#endif