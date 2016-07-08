/*
  Multi-sensor input connected to mangOH platform
  The program detects the following sensors:
  a. Temperature humidity using DHT22 on pin D2 (http://www.seeedstudio.com/wiki/Grove_-_Temperature_and_Humidity_Sensor_Pro)
  b. Dust sensor using Grove Dust sensor on pin D8 (http://www.seeedstudio.com/wiki/Grove_-_Dust_Sensor)
  c. Oxygen sensor using Grove Oxygen sensor on pin A0 (http://www.seeedstudio.com/wiki/Grove_-_Gas_Sensor(O%E2%82%82))
  d. Light sensor using Grove Light sensor on pin A1 (http://www.seeedstudio.com/wiki/Grove_-_Light_Sensor)
  e. Sound sensor using Grove Sound Sensor  on pin A2 ( http://www.seeedstudio.com/wiki/Grove_-_Sound_Sensor)
  f. Water sensor using Grove water sensor on pin D6  (http://www.seeedstudio.com/wiki/Grove_-_Water_Sensor)
  Written by CTO office Sierra
*/


#include <SwiBridge.h>
#include <DHT.h>
#include <Bridge.h>
#include <BridgeClient.h>
#include <BridgeServer.h>
#include <BridgeSSLClient.h>
#include <BridgeUdp.h>
#include <Console.h>
#include <FileIO.h>
#include <HttpClient.h>
#include <Mailbox.h>
#include <Process.h>
#include <YunClient.h>
#include <YunServer.h>

#define AV_URL          "" // replace with url of your airvantage accounr
#define AV_PASSWORD     "" // you can change this here and make it the same as the one on application model
#define APP_ASSET_ID    "sensors"
#define APP_TEMPERATURE "arduino.temperature"
#define APP_HUMIDITY    "arduino.humidity"
#define APP_LUMINOSITY  "arduino.luminosity"
#define APP_NOISE_LEVEL "arduino.noise"
#define APP_WATER       "arduino.water"
#define APP_DUST        "arduino.dust"
#define APP_OXYGEN      "arduino.oxygen"

#define DHTPIN          A0  // what pin we're connected to
#define DUSTPIN         8   // Dust sensor is connected to digital pin 8
#define OXYGENPIN       A3  // Oxygen sensor is connected to pin A3
#define LIGHTPIN        A1  // Light sensor is connected to pin A1
#define SOUNDPIN        A2  // Sound sensor is connected pin A2
#define WATERPIN        6   // Water sensor is on pin 6

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Readings for DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Readings for Oxygen sensor
#define OXYGEN_SENSOR_VERSION 10  // choose as per board provided
#if OXYGEN_SENSOR_VERSION==11
const int AMP = 121;
#elif OXYGEN_SENSOR_VERSION==10
const int AMP = 201;
#endif
const float K_O2 = 6.64;

// Readings for Dust sensor
const unsigned long sampleTimeInMs = 30000;
unsigned long startTime;
unsigned long lowpulseoccupancy = 0;
float dustConcentration;


void setup() {
  Serial.begin(9600);
  Serial.println("mangOH multi-sensors tutorial");

  Serial.println("Starting the bridge");
  Bridge.begin(115200);

  // Initialize the humidity and temperature sensor
  dht.begin();

  Serial.print("Start Session: ");
  Serial.print(AV_URL);
  Serial.print(" ");
  Serial.println(AV_PASSWORD);
  // Push to AirVantage if a non-empty URL is provided
  const bool pushAV = (AV_URL[0] != '\0');
  SwiBridge.startSession(AV_URL, AV_PASSWORD, pushAV, CACHE);

  // Required for dust sensor
  startTime = millis();
}

void loop() {
  // Oxygen sensor calculations
  const float oxygenSensorValue = analogRead(OXYGENPIN);
  float oxygenSensorVoltage = (oxygenSensorValue / 1024.0) * 5.0;
  oxygenSensorVoltage = oxygenSensorVoltage / (float)AMP * 10000.0;
  float oxygenPercentage = oxygenSensorVoltage / K_O2;
  Serial.print("Oxygen level: ");
  Serial.print(oxygenPercentage);
  Serial.println("%");

  // Temperature and humidity
  // Reading temperature or humidity takes about 250 milliseconds!  Sensor readings may also be up
  // to 2 seconds 'old' (its a very slow sensor).
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C");

  // Dust sensor
  const unsigned long durationInMicroseconds = pulseIn(DUSTPIN, LOW);
  lowpulseoccupancy += durationInMicroseconds;
  if ((millis() - startTime) >= sampleTimeInMs) // if the sample time >= 30s
  {
    const float ratio = lowpulseoccupancy / (sampleTimeInMs * 10.0); // Integer percentage 0 to 100
    // using spec sheet curve
    dustConcentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
    Serial.print("Dust concentration: ");
    Serial.print(dustConcentration);
    Serial.println(" pcs/0.01cf");

    // Reset variables for next sample
    lowpulseoccupancy = 0;
    startTime = millis();
  }

  // Light sensor
  const int lightSensorValue = analogRead(LIGHTPIN);
  const float lightSensorResistance = (float)(1023 - lightSensorValue) * 10 / lightSensorValue;
  Serial.print("Light sensor reading: ");
  Serial.println(lightSensorValue);
  Serial.print("Light sensor resistance: ");
  Serial.println(lightSensorResistance, DEC); //show the light intensity on the serial monitor

  // Sound sensor
  const int soundSensorValue = analogRead(SOUNDPIN);
  Serial.print("Sound sensor: ");
  Serial.println(soundSensorValue);

  // Water sensor
  const bool waterPresent = (digitalRead(WATERPIN) == LOW);
  Serial.print("Water present: ");
  Serial.println(waterPresent);

  SwiBridge.pushFloat(APP_ASSET_ID "." APP_HUMIDITY, 3, h);
  SwiBridge.pushFloat(APP_ASSET_ID "." APP_TEMPERATURE, 3, t);
  SwiBridge.pushFloat(APP_ASSET_ID "." APP_DUST, 3, dustConcentration);
  SwiBridge.pushFloat(APP_ASSET_ID "." APP_OXYGEN, 3, oxygenPercentage);
  SwiBridge.pushInteger(APP_ASSET_ID "." APP_LUMINOSITY, lightSensorResistance);
  SwiBridge.pushInteger(APP_ASSET_ID "." APP_NOISE_LEVEL, soundSensorValue);
  SwiBridge.pushBoolean(APP_ASSET_ID "." APP_WATER, waterPresent);

  delay(20000);
}

