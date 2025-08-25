#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "debug_system.h"

class SensorManager {
private:
    static OneWire oneWire;
    static DallasTemperature tempSensor;
    
public:
    static void init();
    static void update();
    static float readTemperature();
    static bool isTemperatureSensorConnected();
};

#endif // SENSOR_MANAGER_H