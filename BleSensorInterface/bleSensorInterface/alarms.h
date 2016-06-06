#ifndef ALARMS_H
#define ALARMS_H

void checkShockAlarm(double shockValue);
void checkTemperatureAlarm(double temperature);
void checkHumidityAlarm(double humidity);
void checkLuminosityAlarm(double luminosity);
void checkOrientationAlarm(int32_t orientation);

#endif // ALARMS_H

