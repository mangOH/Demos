#ifndef SENSOR_CONVERSIONS_H
#define SENSOR_CONVERSIONS_H

#include <stdint.h>

// Accelerometer ranges
#define ACC_RANGE_2G 0
#define ACC_RANGE_4G 1
#define ACC_RANGE_8G 2
#define ACC_RANGE_16G 3

void convertIRTemperatureSensorData(
    const uint8_t* data, double* ambientTemperature, double* objectTemperature);
void convertHumiditySensorData(const uint8_t* data, double* temperature, double* humidity);
void convertBarometricPressureSensorData(
    const uint8_t* data, double* temperature, double* pressure);
void convertOpticalSensorData(const uint8_t* data, double* luminosity);
void convertMovementSensorData(
    const uint8_t* data,
    int accelerometerRangeSetting,
    double* accX,
    double* accY,
    double* accZ,
    double* gyroX,
    double* gyroY,
    double* gyroZ,
    double* magX,
    double* magY,
    double* magZ);

#endif // SENSOR_CONVERSIONS_H

