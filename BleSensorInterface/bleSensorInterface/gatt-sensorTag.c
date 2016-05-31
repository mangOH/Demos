/*
 *
 *  Tool: gatt-sensorTag
 * 
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  TI sensorTag sensor data output on top of gatttool
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>
#include <sys/signalfd.h>

#include <readline/readline.h>
#include <readline/history.h>

enum AlarmCmd {
	ALARMCMD_LED_OFF = 0,
	ALARMCMD_LED_RED = 1,
	ALARMCMD_LED_GREEN = 2,
	ALARMCMD_BUZZ = 4
};


//#define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
#define PRINT_DEBUG(_fmt_, ...) ( printf(_fmt_, __VA_ARGS__) )
#else
#define PRINT_DEBUG(_fmt_, ...) ( )
#endif

/* supposed the Move sensor raw data has the maximum value array */
#define MAX_SENSOR_RAW	20

/* Vincent added for TI SensorTag data calculation and AirVantage on mangOH board */

static int writeSensorDataToFile(const char* fileName, char *data)
{
	FILE *fp;
	char *dir = "sensorTag";
	char path[64];

	if (!data)
		exit(1);

	sprintf(path, "%s", fileName); 
//	printf("writeSensorDataToFile: %s\n", path);
	fp=fopen(fileName, "w");
	if(fp==NULL){
		printf("Error!");
		exit(1);
	}
//	printf("\tWriting data: %s\n", data);
	fprintf(fp,"%s",data);

	fclose(fp);
	return 0;
}

static float sensorMpu9250GyroConvert(int16_t data)
{
	//-- calculate rotation, unit deg/s, range -250, +250
	return (data * 1.0) / (65536 / 500);
}

static float sensorMpu9250MagConvert(int16_t data)
{
	//-- calculate magnetism, unit uT, range +-4900
	return 1.0 * data;
}

// Accelerometer ranges
#define ACC_RANGE_2G      0
#define ACC_RANGE_4G      1
#define ACC_RANGE_8G      2
#define ACC_RANGE_16G     3

static int accRange = ACC_RANGE_16G;

/* Move sensor Accelerometer convert */
static float sensorMpu9250AccConvert(int16_t rawData)
{
  float v;

  switch (accRange)
  {
  case ACC_RANGE_2G:
    //-- calculate acceleration, unit G, range -2, +2
    v = (rawData * 1.0) / (32768/2);
    break;

  case ACC_RANGE_4G:
    //-- calculate acceleration, unit G, range -4, +4
    v = (rawData * 1.0) / (32768/4);
    break;

  case ACC_RANGE_8G:
    //-- calculate acceleration, unit G, range -8, +8
    v = (rawData * 1.0) / (32768/8);
    break;

  case ACC_RANGE_16G:
    //-- calculate acceleration, unit G, range -16, +16
    v = (rawData * 1.0) / (32768/16);
    break;
  }

  return v;
}

/* Humidity Sensor */
static void sensorHdc1000Convert(uint16_t rawTemp, uint16_t rawHum,
                        float *temp, float *hum)
{
	//-- calculate temperature [°C]
	*temp = ((double)(int16_t)rawTemp / 65536)*165 - 40;
 
	//-- calculate relative humidity [%RH]
	*hum = ((double)rawHum / 65536)*100;
}

/* Barometic Pressure Sensor */
static float calcBmp280(unsigned int rawValue)
{
  return rawValue / 100.0f;
}

/* IR Temperature convert */
static void sensorTmp007Convert(int16_t rawAmbTemp, int16_t rawObjTemp, float *tAmb, float *tObj)
{
	const float SCALE_LSB = 0.03125;
	float t;
	int it;
 
	it = (int)((rawObjTemp) >> 2);
	t = ((float)(it)) * SCALE_LSB;
	*tObj = t;
 
	it = (int)((rawAmbTemp) >> 2);
	t = (float)it;
	*tAmb = t * SCALE_LSB;
}

float sensorOpt3001Convert(int16_t rawData)
{
	int16_t e, m;
 
	m = rawData & 0x0FFF;
	e = (rawData & 0xF000) >> 12;
 
	//printf("m = %d, e = %d\n", m, e);
	if (e == 0)
		return m * (0.01 * 1);
	else if(e == 1)
		return m * (0.01 * 2.0 * 1);
	else if(e == 2)
		return m * (0.01 * 2.0 * 2);
	else
		return m * (0.01 * 2.0 * 3);
	//return m * (0.01 * pow(2.0,e));
}

static int rawSensorDataStr2Value(char *line, int *len, int **value)
{
	char *token, *newtoken;
	char *search = ":";
	int i;
	int index = 0;
	int new;

	PRINT_DEBUG("Parsing: %s", line);

	// Token will point to "Characteristic value/descriptor".
	token = strtok(line, search);
//	printf("token: %s\n", token);

	// Token will point to "2d ff .....".
	token = strtok(NULL, search);
//	printf("token: %s\n", token);

	search = " ";
	newtoken = strtok(token, search);
//	printf("new token: %s\n", newtoken);
	sscanf(newtoken, "%x", &new);
	*(*value+index) = new;
	index++;

	while(newtoken) {
		newtoken = strtok(NULL, search);
		if (newtoken == NULL) {
//			printf("empty reached!!!\n");
			break;
		}
//		printf("new token: %s\n", newtoken);
		sscanf(newtoken, "%x", &new);
		if (index >= MAX_SENSOR_RAW)
			break;
		*(*value+index) = new;
		index++;
	} 
	*len = index;

	return 0;
}
int main(int argc, char *argv[])
{
	FILE *fp, *alarmFp;
	//const char *mac = "A0:E6:F8:C3:20:00";
	char *mac = "A0:E6:F8:B6:1F:00";
	//const char *mac = "A0:E6:F8:C1:76:06";
	const char *tool = "./gatttool";
	int i, len;
	int rawValue[MAX_SENSOR_RAW];
	int *p = rawValue;
	bool flag = true;
        int16_t data1;
        int16_t data2;
        int16_t data3;
	float value1, value2, value3;
	float lastValue1, lastValue2, lastValue3;
	float accDiff, accDiffX, accDiffY, accDiffZ;
	int i1,i2,i3, index;
	unsigned int d32_1, d32_2;
	double compassAngle;
	int alarm;
	int initFlag = 0;
	enum AlarmCmd alarmCmd = ALARMCMD_LED_OFF;
	enum AlarmCmd lastAlarmCmd = ALARMCMD_LED_OFF;
	char data[64];
	char path[256];
	char cmd[1024];
	char sensor[2048];
	char json_result[2048];
	const char *alarmFile = "./alarm.cmd";
	

	if (argc > 1) {
		if (argc == 2) {
			printf("argv[1] = %s\n", argv[1]);
			mac = argv[1];
		} else {
			printf("Wrong parameter\n");
			exit(0);
		}
	}

	snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x24 -n 01;%s  -b %s --char-write -a 0x3c -n ff00; %s  -b %s --char-write -a 0x34 -n 01;%s  -b %s --char-write -a 0x44 -n 01;%s  -b %s --char-write -a 0x2c -n 01;%s  -b %s --char-write -a 0x50 -n 01;", tool, mac, tool, mac, tool, mac, tool, mac, tool, mac, tool, mac);
	printf("Running: %s\n", cmd);
	system(cmd);

	/* Wait until all sensor RAW data ready to be read */
	while(flag) {
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x21;%s  -b %s --char-read -a 0x39; %s  -b %s --char-read -a 0x31;%s -b %s --char-read -a 0x41;%s  -b %s --char-write -a 0x29;", tool, mac, tool, mac, tool, mac, tool, mac, tool, mac);

		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}

		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			if (rawValue[0] == 0 && rawValue[1] == 0)
				break;
			else {
				/* Sensor data ready flag */
				flag = false;
			}
		}
		sleep(1);
	}

	alarmFp = fopen(alarmFile, "w+");
	if (alarmFp == NULL) {
		printf("!!!!!!Failed to open %s!\n", alarmFile);
	}
	PRINT_DEBUG("Successfully open Alarm file %s!\n", alarmFile);

	printf("\n****** Sensor data available to proccess! ****** \n\n");
	
	while(1) {
		snprintf(json_result, sizeof(json_result), "{");
		/* IR Temperature Sensor */
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x21", tool, mac);
		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			PRINT_DEBUG("len = %d\n", len);
			for (i = 0; i<len; i++) {
				PRINT_DEBUG("OUT: %.2x\n", rawValue[i]);
			}
		}
		data1 = rawValue[0] | rawValue[1] << 8;
                data2 = rawValue[2] | rawValue[3] << 8;
                sensorTmp007Convert(data1, data2, &value1, &value2);
                PRINT_DEBUG("IR Temperature = %.2f , Ambient Temperature = %.2f\n", value1, value2);
                snprintf(data, sizeof(data), "IR Temperature: %.2f\nAmbient Temperature: %.2f\n", value1, value2);
                writeSensorDataToFile("IR-temperature.data", data);
		snprintf(sensor, sizeof(sensor), "%s\"IRTemperatureSensor\":{\"IR Temperature\":%.2f,\"Ambient Temperature\":%.2f},", json_result, value1, value2);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		/* Move Sensor */
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x39", tool, mac);
		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			PRINT_DEBUG("len = %d\n", len);
			for (i = 0; i<len; i++) {
				PRINT_DEBUG("OUT: %.2x\n", rawValue[i]);
			}
		}
	        data1 = rawValue[0] | rawValue[1] << 8;
	        data2 = rawValue[2] | rawValue[3] << 8;
	        data3 = rawValue[4] | rawValue[5] << 8;
        	PRINT_DEBUG("Converting Gyro data: 0x%2x, 0x%2x, 0x%2x\n", data1, data2, data3);
        	value1 = sensorMpu9250GyroConvert(data1);
        	value2 = sensorMpu9250GyroConvert(data2);
        	value3 = sensorMpu9250GyroConvert(data3);
        	PRINT_DEBUG("Gyro X = %.1f/S\n", value1);
        	PRINT_DEBUG("Gyro Y = %.1f/S\n", value2);
        	PRINT_DEBUG("Gyro Z = %.1f/S\n", value3);
		snprintf(data, sizeof(data), "X=%.1f/S; Y=%.1f/S; Z=%.1f/S", value1, value2, value3); 
		writeSensorDataToFile("Gyro.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"MoveSensor\":{\"Gyroscope\":{\"X\":%.1f,\"Y\":%.1f,\"Z:\":%.1f},", json_result, value1, value2, value3);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

	        data1 = rawValue[12] | rawValue[13] << 8;
	        data2 = rawValue[14] | rawValue[15] << 8;
	        data3 = rawValue[16] | rawValue[17] << 8;
        	PRINT_DEBUG("Converting Mag data: 0x%2x, 0x%2x, 0x%2x\n", data1, data2, data3);
        	value1 = sensorMpu9250MagConvert(data1);
        	value2 = sensorMpu9250MagConvert(data2);
        	value3 = sensorMpu9250MagConvert(data3);
        	PRINT_DEBUG("Mag X = %.2f uT\n", value1);
        	PRINT_DEBUG("Mag Y = %.2f uT\n", value2);
        	PRINT_DEBUG("Mag Z = %.2f uT\n", value3);
		snprintf(data, sizeof(data), "X=%.2fuT; Y=%.2fuT; Z=%.2fuT", value1, value2, value3); 
		writeSensorDataToFile("Mag.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"Magnetometer\":{\"X\":%.1f,\"Y\":%.1f,\"Z:\":%.1f},", json_result, value1, value2, value3);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		/* MagnetoVector = (x,y,0)
		 * Angle = (atan(y/x)/Pi) * 360;
		 */
		if (value1 == 0.0) 
			compassAngle = 0.0;
		else
			compassAngle = (atan(value2/value1) * 360.0)/3.14;
		PRINT_DEBUG("Compass Angle = %.2f\n", compassAngle);
		snprintf(data, sizeof(data), "%.2f", compassAngle); 
		writeSensorDataToFile("Compass.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"Compass\":{\"Angle\":%.2f},", json_result, compassAngle);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

	        data1 = rawValue[6] | rawValue[7] << 8;
	        data2 = rawValue[8] | rawValue[9] << 8;
	        data3 = rawValue[10] | rawValue[11] << 8;
        	PRINT_DEBUG("Converting Acc data: 0x%2x, 0x%2x, 0x%2x\n", data1, data2, data3);
        	value1 = sensorMpu9250AccConvert(data1);
        	value2 = sensorMpu9250AccConvert(data2);
        	value3 = sensorMpu9250AccConvert(data3);
        	PRINT_DEBUG("Accelerometer X = %.2f g\n", value1);
        	PRINT_DEBUG("Accelerometer Y = %.2f g\n", value2);
        	PRINT_DEBUG("Accelerometer Z = %.2f g\n", value3);
		snprintf(data, sizeof(data), "X=%.2fg; Y=%.2fg; Z=%.2fg", value1, value2, value3); 
		writeSensorDataToFile("Accelerometer.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"Accelerometer\":{\"X\":%.1f,\"Y\":%.1f,\"Z:\":%.1f},", json_result, value1, value2, value3);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		/* Shock: 
		 * Like to set an alarm if a shock or rapid acceleration has happened to our sensor
		 *
		 * Compute difference between vectors (SensAcc2 – SensAcc1)
		 * SensAccDiff = (x2-x1, y2-y1, z2-z1) = (dx,dy,dz)

		 * Compute vector magnitude
		 * |SensAccDiff| = sqrt(dx^2 + dy^2 + dz^2)
		 * If |SensAccDiff| > SomeMaxValue then set_alarm_flag
		 */
		accDiffX = value1 - lastValue1;
		accDiffY = value2 - lastValue2;
		accDiffZ = value3 - lastValue3;
		accDiff = sqrt(accDiffX*accDiffX + accDiffY*accDiffY + accDiffZ*accDiffZ);
		PRINT_DEBUG("Last Accelerometer x = %.2f, y = %.2f, z = %.2f\n", lastValue1, lastValue2, lastValue3);
		PRINT_DEBUG("Shock = %.2f, dx=%.2f, dy=%.2f, dz=%.2f \n", accDiff, accDiffX, accDiffY, accDiffZ);
		snprintf(data, sizeof(data), "%.2f", accDiff); 
		writeSensorDataToFile("Shock.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"Shock Movement\":{\"Shock\":%.2f},", json_result, accDiff);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		lastValue1 = value1;
		lastValue2 = value2;
		lastValue3 = value3;

		i1 = (int)value1;
		i2 = (int)value2;
		i3 = (int)value3;
		if ((i1 == 1) || (i1 == 2)) {
			i1 = 1;
			index = 1;
		}
		else if ((i1 == -1) || (i1 == -2)) {
			i1 = -1;
			index = 2;
		}
		else if ((i2 == 1) || (i2 == 2)) {
			i2 = 1;
			index = 3;
		}
		else if ((i2 == -1) || (i2 == -2)) {
			i2 = -1;
			index = 4;
		}
		else if ((i3 == 1) || (i3 == 2)) {
			i3 = 1;
			index = 5;
		}
		else if ((i3 == -1) || (i3 == -2)) {
			i3 = -1;
			index = 6;
		}
		snprintf(data, sizeof(data), "%d. x=%dg; y=%dg; z=%dg", index, i1, i2, i3); 
		writeSensorDataToFile("Acc-orientation.data", data); 
		snprintf(sensor, sizeof(sensor), "%s\"Orientation\":{\"Index\":%d,\"X\":%d,\"Y\":%d,\"Z:\":%d}},", json_result, index, i1, i2, i3);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		//snprintf(json_result, sizeof(json_result), "%s}\n", sensor);
		//printf("JSON:\n %s", json_result);

		/* Barometric Pressure sensor: */
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x31", tool, mac);
		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			PRINT_DEBUG("len = %d\n", len);
			for (i = 0; i<len; i++) {
				PRINT_DEBUG("OUT: %.2x\n", rawValue[i]);
			}
		}
		d32_1 = rawValue[0] | rawValue[1] << 8 | rawValue[2] << 16;
                d32_2 = rawValue[3] | rawValue[4] << 8 | rawValue[5] << 16;
                value1 = calcBmp280(d32_1);
                value2 = calcBmp280(d32_2);
                PRINT_DEBUG("Pressure Temperature = %.2f , Pressure = %.2f\n", value1, value2);
                snprintf(data, sizeof(data), "Pressure Temperature: %.2f\nPressure: %.2f\n", value1, value2);
                writeSensorDataToFile("Pressure.data", data);
		snprintf(sensor, sizeof(sensor), "%s\"BarometricPressureSensor\":{\"Pressure Temperature\":%.2f,\"Pressure\":%.2f},", json_result, value1, value2);
		snprintf(json_result, sizeof(json_result), "%s", sensor);

		/* Optical sensor: */
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x41", tool, mac);
		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			PRINT_DEBUG("len = %d\n", len);
			for (i = 0; i<len; i++) {
				PRINT_DEBUG("OUT: %.2x\n", rawValue[i]);
			}
		}
		data1 = rawValue[0] | rawValue[1] << 8;
                value1 = sensorOpt3001Convert(data1);
                PRINT_DEBUG("Optical sensor = %.2f\n", value1);
                snprintf(data, sizeof(data), "Optical sensor: %.2f\n", value1);
                writeSensorDataToFile("Optical.data", data);
		snprintf(sensor, sizeof(sensor), "%s\"OpticalSensor\":{\"Luminosity\":%.2f},", json_result, value1);
		snprintf(json_result, sizeof(json_result), "%s\n", sensor);

		/* Humidity sensor: */
		snprintf(cmd, sizeof(cmd), "%s  -b %s --char-read -a 0x29", tool, mac);
		PRINT_DEBUG("Running: %s\n", cmd);
		fp = popen(cmd, "r");
		if (fp == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			rawSensorDataStr2Value(path, &len, &p);
			PRINT_DEBUG("len = %d\n", len);
			for (i = 0; i<len; i++) {
				PRINT_DEBUG("OUT: %.2x\n", rawValue[i]);
			}
		}
		data1 = rawValue[0] | rawValue[1] << 8;
		data2 = rawValue[2] | rawValue[3] << 8;
                sensorHdc1000Convert(data1, data2, &value1, &value2);
                PRINT_DEBUG("Humidity sensor temp = %.2f, humidity = %.2f\n", value1, value2);
                snprintf(data, sizeof(data), "Humidity Temperature: %.2f\n Humidity: %.2f\n", value1, value2);
                writeSensorDataToFile("Humidity.data", data);
		snprintf(sensor, sizeof(sensor), "%s\"HumiditySensor\":{\"Temperature\":%.2f,\"Humidity\":%.2f}", json_result, value1, value2);
		snprintf(json_result, sizeof(json_result), "%s}\n", sensor);

		printf("%s", json_result);
                writeSensorDataToFile("sensorTag.json", json_result);

		if (initFlag == 0) {
			snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x4e -n 00;", tool, mac);
			printf("\nInit Alarm cmd: %s, LED off\n\n", cmd);
			system(cmd);
			initFlag = 1;
		}

		/* Read the output a line at a time - output it. */
		if (alarmFp) {
			fseek(alarmFp, 0, SEEK_SET);
			while (fgets(path, sizeof(path)-1, alarmFp) != NULL) {
				alarmCmd = atoi(path);
				PRINT_DEBUG("***********Alarm FP: path = %s, alarm=%d\n", path, alarmCmd);
			}
			if (lastAlarmCmd == alarmCmd)
				continue;
			lastAlarmCmd = alarmCmd;
				
			switch(alarmCmd) {
				case ALARMCMD_LED_OFF:
				default: 
					snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x4e -n 00;", tool, mac);
					printf("\nReceived and run Alarm cmd: %s, LED off\n\n", cmd);
					system(cmd);
					break;

				case ALARMCMD_LED_RED:
					snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x4e -n 01;", tool, mac);
					printf("\nReceived and run Alarm cmd: %s, LED red\n\n", cmd);
					system(cmd);
					break;

				case ALARMCMD_LED_GREEN:
					snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x4e -n 02;", tool, mac);
					printf("\nReceived and run Alarm cmd: %s, LED green\n\n", cmd);
					system(cmd);
					break;

				case ALARMCMD_BUZZ:
					snprintf(cmd, sizeof(cmd), "%s  -b %s --char-write -a 0x4e -n 04;", tool, mac);
					printf("\nReceived and run Alarm cmd: %s, Buzz\n\n", cmd);
					system(cmd);
					break;
			}
		}
		sleep(1);
	}
	/* close */
	fclose(alarmFp);
	pclose(fp);
}

