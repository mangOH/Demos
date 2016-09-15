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
    revision="2.0.0">
  <capabilities>
    <communication><protocol comm-id="IMEI" type="MQTT"/></communication>
    <data>
      <encoding type="MQTT">
        <asset default-label="Application" id="a">
          <node default-label="Bluetooth SensorTag" path="bt">
            <node default-label="Commands" path="bluetooth">
              <command default-label="Buzzer" id="buzzer">
                <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
              </command>
              <command default-label="Red LED" id="redLED">
                <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
              </command>
              <command default-label="Green LED" id="greenLED">
                <parameter default-label="Enable" default-value="true" id="enable" type="boolean"/>
              </command>
            </node>
            <node default-label="Alarms" path="alarm">
              <variable default-label="Alarms/Humidity" path="humidity" type="boolean"/>
              <variable default-label="Alarms/Luminosity" path="luminosity" type="boolean"/>
              <variable default-label="Alarms/Orientation" path="orientation" type="boolean"/>
              <variable default-label="Alarms/Temperature" path="temperature" type="boolean"/>
              <variable default-label="Alarms/Shock" path="shock" type="double"/>
            </node>
            <node default-label="Calculations" path="calc">
              <variable default-label="Calculations/Compass" path="compass" type="double"/>
              <variable default-label="Calculations/Shock" path="shock" type="double"/>
              <variable default-label="Calculations/Orientation" path="orientation" type="int"/>
            </node>
            <node default-label="Readings" path="read">
              <node default-label="Movement" path="movement">
                <node default-label="Accelerometer" path="accelerometer">
                  <variable
                      default-label="Bluetooth/Movement/Accelerometer/X" path="x" type="double"/>
                  <variable
                      default-label="Bluetooth/Movement/Accelerometer/Y" path="y" type="double"/>
                  <variable
                      default-label="Bluetooth/Movement/Accelerometer/Z" path="z" type="double"/>
                </node>
                <node default-label="Magnetometer" path="magnetometer">
                  <variable
                      default-label="Bluetooth/Movement/Magnetometer/X" path="x" type="double"/>
                  <variable
                      default-label="Bluetooth/Movement/Magnetometer/Y" path="y" type="double"/>
                  <variable
                      default-label="Bluetooth/Movement/Magnetometer/Z" path="z" type="double"/>
                </node>
                <node default-label="Gyroscope" path="gyroscope">
                  <variable default-label="Bluetooth/Movement/Gyroscope/X" path="x" type="double"/>
                  <variable default-label="Bluetooth/Movement/Gyroscope/Y" path="y" type="double"/>
                  <variable default-label="Bluetooth/Movement/Gyroscope/Z" path="z" type="double"/>
                </node>
              </node>
              <node default-label="IR Temperature" path="ir">
                <variable
                    default-label="Bluetooth/IR/Ambient Temperature"
                    path="ambientTemperature"
                    type="double"/>
                <variable
                    default-label="Bluetooth/IR/Object Temperature"
                    path="objectTemperature"
                    type="double"/>
              </node>
              <node default-label="Humidity" path="humidity">
                <variable
                    default-label="Bluetooth/Humidity/Relative Humidity"
                    path="humidity"
                    type="double"/>
                <variable
                    default-label="Bluetooth/Humidity/Temperature"
                    path="temperature"
                    type="double"/>
              </node>
              <node default-label="Barometric Pressure" path="barometer">
                <variable
                    default-label="Bluetooth/Barometric Pressure/Temperature"
                    path="temperature"
                    type="double"/>
                <variable
                    default-label="Bluetooth/Barometric Pressure/Pressure"
                    path="pressure"
                    type="double"/>
              </node>
              <variable default-label="Bluetooth/Luminosity" path="luminosity" type="double"/>
            </node>
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
