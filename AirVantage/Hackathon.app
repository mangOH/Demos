<?xml version="1.0" encoding="ISO-8859-1" ?>
<!--
This application model provides sensor data from a TI SensorTag, a number of Arduino Sensors and
the GPS of the WP85.  The verbose labeling of the variables is due to a limitation in the
AirVantage web UI where only the label of the variable is shown in the data timeline rather than
the complete path to the variable.
-->
<app:application
    xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0"
    type="io.mangoh.demo.hackathon.app"
    name="mangOH Hackathon Demo"
    revision="1.0.1">
  <capabilities>
    <communication><protocol comm-id="IMEI" type="MQTT"/></communication>
    <data>
      <encoding type="MQTT">
        <asset default-label="Sensors" id="sensors">
          <node default-label="Bluetooth SensorTag" path="bluetooth">
            <command default-label="Red SensorTag LED" id="redLed">
              <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
            </command>
            <command default-label="Green SensorTag LED" id="greenLed">
              <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
            </command>
            <command default-label="SensorTag Buzzer" id="buzzer">
              <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
            </command>
            <node default-label="Motion Sensor" path="motion">
              <node default-label="Accelerometer" path="accelerometer">
                <variable default-label="SensorTag/Motion/Accelerometer/X" path="x" type="double"/>
                <variable default-label="SensorTag/Motion/Accelerometer/Y" path="y" type="double"/>
                <variable default-label="SensorTag/Motion/Accelerometer/Z" path="z" type="double"/>
              </node>
              <node default-label="Gyroscope" path="gyroscope">
                <variable default-label="SensorTag/Motion/Gyroscope/X" path="x" type="double"/>
                <variable default-label="SensorTag/Motion/Gyroscope/Y" path="y" type="double"/>
                <variable default-label="SensorTag/Motion/Gyroscope/Z" path="z" type="double"/>
              </node>
              <node default-label="Magnetometer" path="magnetometer">
                <variable default-label="SensorTag/Motion/Magnetometer/X" path="x" type="double"/>
                <variable default-label="SensorTag/Motion/Magnetometer/Y" path="y" type="double"/>
                <variable default-label="SensorTag/Motion/Magnetometer/Z" path="z" type="double"/>
              </node>
            </node>
            <node default-label="IR Temperature Sensor" path="ir">
              <variable
                  default-label="SensorTag/IR/Ambient Temperature"
                  path="ambientTemperature"
                  type="double"/>
              <variable
                  default-label="SensorTag/IR/Object Temperature"
                  path="objectTemperature"
                  type="double"/>
            </node>
            <node default-label="Humidity Sensor" path="humidity">
              <variable
                  default-label="SensorTag/Humidity/Relative Humidity"
                  path="humidity"
                  type="double"/>
              <variable
                  default-label="SensorTag/Humidity/Temperature"
                  path="temperature"
                  type="double"/>
            </node>
            <node default-label="Barometric Pressure Sensor" path="barometer">
              <variable
                  default-label="SensorTag/Barometer/Temperature"
                  path="temperature"
                  type="double"/>
              <variable
                  default-label="SensorTag/Barometer/Barometric Pressure"
                  path="pressure"
                  type="double"/>
            </node>
            <variable default-label="SensorTag/Luminosity" path="luminosity" type="double"/>
            <variable default-label="SensorTag/Shock" path="shock" type="double"/>
            <variable default-label="SensorTag/Orientation" path="orientation" type="int"/>
            <variable default-label="SensorTag/Compass Angle" path="compass" type="double"/>
          </node>
          <node default-label="Arduino Sensors" path="arduino">
            <variable default-label="Arduino/Temperature" path="temperature" type="double"/>
            <variable default-label="Arduino/Humidity" path="humidity" type="double"/>
            <variable default-label="Arduino/Luminosity" path="luminosity" type="int"/>
            <variable default-label="Arduino/Noise" path="noise" type="int"/>
            <variable default-label="Arduino/Water" path="water" type="boolean"/>
            <variable default-label="Arduino/Dust" path="dust" type="double"/>
            <variable default-label="Arduino/Oxygen" path="oxygen" type="double"/>
          </node>
          <node default-label="mangOH Sensors" path="mangoh">
            <node default-label="GPS" path="gps">
              <variable default-label="mangOH/GPS/Latitude" path="latitude" type="double"/>
              <variable default-label="mangOH/GPS/Longitude" path="longitude" type="double"/>
            </node>
          </node>
        </asset>
      </encoding>
    </data>
  </capabilities>
</app:application>
