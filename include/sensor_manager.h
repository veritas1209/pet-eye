#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "config.h"
#include <Wire.h>

class SensorManager {
private:
    unsigned long lastSensorRead;
    unsigned long lastI2CScan;

public:
    SensorManager();
    bool init();
    void update();
    void readTemperature();
    void scanI2CDevices();
    void printSensorStatus();
    String getSensorJSON();
};

extern SensorManager sensorManager;

#endif