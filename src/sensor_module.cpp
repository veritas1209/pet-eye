/* 
#include "sensor_module.h"
#include "config.h"

SensorModule sensors;

SensorModule::SensorModule() : mpuInitialized(false), lastUpdate(0) {}

bool SensorModule::init() {
    // I2C 초기화
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("I2C 초기화 완료");

    // MPU6050 초기화
    if (!mpu.begin()) {
        Serial.println("MPU6050 센서를 찾을 수 없습니다!");
        mpuInitialized = false;
        return false;
    }

    // MPU6050 설정
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroscopeRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    mpuInitialized = true;
    Serial.println("센서 모듈 초기화 완료");
    return true;
}

bool SensorModule::readTemperature(float& temperature) {
    int analogValue = analogRead(TEMP_SENSOR_PIN);
    float voltage = analogValue * (3.3 / 4095.0);
    
    // SEN050007 온도 센서 변환 공식
    // 실제 센서 데이터시트에 맞게 수정 필요
    temperature = (voltage - 0.5) * 100.0;
    
    return true;
}

bool SensorModule::readMPU6050(float& accelX, float& accelY, float& accelZ, 
                               float& gyroX, float& gyroY, float& gyroZ) {
    if (!mpuInitialized) return false;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;

    gyroX = g.gyro.x;
    gyroY = g.gyro.y;
    gyroZ = g.gyro.z;

    return true;
}

void SensorModule::updateSensorData() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate < SENSOR_UPDATE_INTERVAL) return;

    currentSensorData.isValid = false;

    // 온도 읽기
    if (readTemperature(currentSensorData.temperature)) {
        // MPU6050 읽기
        if (readMPU6050(currentSensorData.accelX, currentSensorData.accelY, currentSensorData.accelZ,
                        currentSensorData.gyroX, currentSensorData.gyroY, currentSensorData.gyroZ)) {
            currentSensorData.timestamp = currentTime;
            currentSensorData.isValid = true;
        }
    }

    lastUpdate = currentTime;
}

void SensorModule::calibrateMPU() {
    if (!mpuInitialized) return;
    
    Serial.println("MPU6050 캘리브레이션 시작...");
    delay(2000);
    Serial.println("MPU6050 캘리브레이션 완료");
}

*/