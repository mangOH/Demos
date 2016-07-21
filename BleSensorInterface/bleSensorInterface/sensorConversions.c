#include "sensorConversions.h"
#include <math.h>

static double convertMovementSensorGyroReading(int16_t data);
static double convertMovementSensorMagReading(int16_t data);
static double convertMovementSensorAccReading(int16_t rawData, int accelerometerRangeSetting);


void convertIRTemperatureSensorData
(
    const uint8_t* data,
    double* ambientTemperature,
    double* objectTemperature
)
{
    int16_t ambientReading = (data[2] | (data[3] << 8));
    int16_t objectReading = (data[0] | (data[1] << 8));

    *ambientTemperature = ((double)ambientReading) / 128;
    *objectTemperature = ((double)objectReading) / 128;
}

void convertHumiditySensorData
(
    const uint8_t* data,
    double* temperature,
    double* humidity
)
{
    uint16_t temperatureReading = (data[0] | (data[1] << 8));
    uint16_t humidityReading = (data[2] | (data[3] << 8));

    //-- calculate temperature [°C]
    *temperature = (((double)(int16_t)temperatureReading / 65536) * 165) - 40;

    //-- calculate relative humidity [%RH]
    *humidity = ((double)humidityReading / 65536) * 100;
}

void convertBarometricPressureSensorData
(
    const uint8_t* data,
    double* temperature,
    double* pressure
)
{
    // TODO: wiki says that both temperature and pressure are unsigned values, but that means that
    // the temperature sensor can't handle negative temperatures.  Is this correct?
    uint32_t temperatureReading = (data[0] | (data[1] << 8) | (data[2] << 16));
    uint32_t pressureReading = (data[3] | (data[4] << 8) | (data[2] << 16));
    *temperature = ((double)temperatureReading) / 100;
    *pressure = ((double)pressureReading) / 100;
}

void convertOpticalSensorData
(
    const uint8_t* data,
    double* luminosity
)
{
    uint16_t reading = (data[0] | (data[1] << 8));

    int16_t m = reading & 0x0FFF;
    int16_t e = (reading & 0xF000) >> 12;

    // printf("m = %d, e = %d\n", m, e);
    if (e == 0)
    {
        *luminosity = m * (0.01 * 1);
    }
    else if (e == 1)
    {
        *luminosity = m * (0.01 * 2.0 * 1);
    }
    else if (e == 2)
    {
        *luminosity = m * (0.01 * 2.0 * 2);
    }
    else
    {
        *luminosity = m * (0.01 * pow(2.0, e));
    }
}

void convertMovementSensorData
(
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
    double* magZ
)
{
    *gyroX = convertMovementSensorGyroReading(data[0] | (data[1] << 8));
    *gyroY = convertMovementSensorGyroReading(data[2] | (data[3] << 8));
    *gyroZ = convertMovementSensorGyroReading(data[4] | (data[5] << 8));
    *accX = convertMovementSensorAccReading(data[6] | (data[7] << 8), accelerometerRangeSetting);
    *accY = convertMovementSensorAccReading(data[8] | (data[9] << 8), accelerometerRangeSetting);
    *accZ = convertMovementSensorAccReading(data[10] | (data[11] << 8), accelerometerRangeSetting);
    *magX = convertMovementSensorMagReading(data[12] | (data[13] << 8));
    *magY = convertMovementSensorMagReading(data[14] | (data[15] << 8));
    *magZ = convertMovementSensorMagReading(data[16] | (data[17] << 8));
}

static double convertMovementSensorGyroReading
(
    int16_t data
)
{
    //-- calculate rotation, unit deg/s, range -250, +250
    return (((double)data) * 500) / 65536;
}

static double convertMovementSensorMagReading
(
    int16_t data
)
{
    //-- calculate magnetism, unit uT, range +-4900
    return (double)data;
}

static double convertMovementSensorAccReading
(
    int16_t rawData,
    int accelerometerRangeSetting
)
{
    double v;

    switch (accelerometerRangeSetting)
    {
        case ACC_RANGE_2G:
            //-- calculate acceleration, unit G, range -2, +2
            v = (((double)rawData) * 2) / 32768;
            break;

        case ACC_RANGE_4G:
            //-- calculate acceleration, unit G, range -4, +4
            v = (((double)rawData) * 4) / 32768;
            break;

        case ACC_RANGE_8G:
            //-- calculate acceleration, unit G, range -8, +8
            v = (((double)rawData) * 8) / 32768;
            break;

        case ACC_RANGE_16G:
            //-- calculate acceleration, unit G, range -16, +16
            v = (((double)rawData) * 16) / 32768;
            break;
    }

    return v;
}
