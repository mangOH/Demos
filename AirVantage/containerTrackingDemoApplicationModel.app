<?xml version="1.0" encoding="ISO-8859-1" ?>
<!--
The default-label attributes of the variables duplicate information in the parent nodes because
AirVantage doesn't display parent node information in the data timeline.  Hopefully AirVantage will
fix this problem at a later date.
-->
<app:application
    xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0"
    type="io.mangoh.demo.container.app"
    name="mangOH Container Demo"
    revision="2.0.0">
  <capabilities>
    <communication><protocol comm-id="IMEI" type="MQTT" /></communication>
    <data>
      <encoding type="MQTT">
        <asset default-label="Container" id="container">
          <command default-label="Buzzer" id="buzzer">
            <parameter default-label="Enable" default-value="true" id="enable" type="boolean" />
          </command>
          <command default-label="Red LED" id="redLED">
            <parameter default-label="Enable" default-value="true" id="enable" type="boolean" />
          </command>
          <command default-label="Green LED" id="greenLED">
            <parameter default-label="Enable" default-value="true" id="enable" type="boolean" />
          </command>
          <node default-label="Alarms" path="alarms">
            <variable default-label="Alarms/Humidity" path="humidity" type="boolean" />
            <variable default-label="Alarms/Luminosity" path="luminosity" type="boolean" />
            <variable default-label="Alarms/Orientation" path="orientation" type="boolean" />
            <variable default-label="Alarms/Temperature" path="temperature" type="boolean" />
            <variable default-label="Alarms/Shock" path="shock" type="double" />
          </node>
          <node default-label="Calculations" path="calculations">
            <variable default-label="Calculations/Compass" path="compass" type="double" />
            <variable default-label="Calculations/Shock" path="shock" type="double" />
            <variable default-label="Calculations/Orientation" path="orientation" type="int" />
          </node>
          <node default-label="Sensors" path="sensors">
            <node default-label="Movement" path="movement">
              <node default-label="Accelerometer" path="accelerometer">
                <variable default-label="Sensors/Movement/Accelerometer/X" path="x" type="double" />
                <variable default-label="Sensors/Movement/Accelerometer/Y" path="y" type="double" />
                <variable default-label="Sensors/Movement/Accelerometer/Z" path="z" type="double" />
              </node>
              <node default-label="Magnetometer" path="magnetometer">
                <variable default-label="Sensors/Movement/Magnetometer/X" path="x" type="double" />
                <variable default-label="Sensors/Movement/Magnetometer/Y" path="y" type="double" />
                <variable default-label="Sensors/Movement/Magnetometer/Z" path="z" type="double" />
              </node>
              <node default-label="Gyroscope" path="gyroscope">
                <variable default-label="Sensors/Movement/Gyroscope/X" path="x" type="double" />
                <variable default-label="Sensors/Movement/Gyroscope/Y" path="y" type="double" />
                <variable default-label="Sensors/Movement/Gyroscope/Z" path="z" type="double" />
              </node>
            </node>
            <node default-label="IR Temperature" path="ir">
              <variable
                  default-label="Sensors/IR/Ambient Temperature"
                  path="ambientTemperature"
                  type="double"/>
              <variable
                  default-label="Sensors/IR/Object Temperature"
                  path="objectTemperature"
                  type="double"/>
            </node>
            <node default-label="Humidity" path="humidity">
              <variable
                  default-label="Sensors/Humidity/Relative Humidity"
                  path="humidity"
                  type="double"/>
              <variable
                  default-label="Sensors/Humidity/Temperature"
                  path="temperature"
                  type="double"/>
            </node>
            <node default-label="Barometric Pressure" path="barometer">
              <variable
                  default-label="Sensors/Barometric Pressure/Temperature"
                  path="temperature"
                  type="double"/>
              <variable
                  default-label="Sensors/Barometric Pressure/Pressure"
                  path="pressure"
                  type="double"/>
            </node>
            <variable default-label="Sensors/Luminosity" path="luminosity" type="double"/>
          </node>
        </asset>
      </encoding>
    </data>
  </capabilities>
</app:application>
