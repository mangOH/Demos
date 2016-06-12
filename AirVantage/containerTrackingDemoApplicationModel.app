<?xml version="1.0" encoding="ISO-8859-1" ?>
<app:application
    xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0"
    type="io.mangoh.demo.container.app"
    name="mangOH Container Demo"
    revision="1.0.2">
  <capabilities>
    <communication><protocol comm-id="IMEI" type="MQTT" /></communication>
    <data>
      <encoding type="MQTT">
        <asset default-label="Container" id="container">
          <command default-label="Start Buzzer" id="startBuzzer">
            <parameter default-label="Status" default-value="true" id="status" type="boolean" />
          </command>
          <command default-label="Command the door" id="doorLED">
            <parameter default-label="Status" default-value="true" id="status" type="boolean" />
          </command>
          <variable default-label="Humidity Alarm" path="alarmHumidity" type="boolean" />
          <variable default-label="Luminosity Alarm" path="alarmLuminosity" type="boolean" />
          <variable default-label="Orientation Alarm" path="alarmOrientation" type="boolean" />
          <variable default-label="Temperature Alarm" path="alarmTemperature" type="boolean" />
          <variable default-label="Shock Alarm" path="alarmShock" type="double" />
          <variable default-label="Compass" path="compass" type="double" />
          <variable default-label="Shock" path="shock" type="double" />
          <variable default-label="Acceleration X" path="accelerationX" type="double" />
          <variable default-label="Acceleration Y" path="accelerationY" type="double" />
          <variable default-label="Acceleration Z" path="accelerationZ" type="double" />
          <variable default-label="Orientation" path="orientation" type="int" />
          <variable default-label="Luminosity" path="luminosity" type="double" />
          <variable default-label="Humidity" path="humidity" type="double" />
          <variable default-label="Temperature" path="temperature" type="double" />
        </asset>
      </encoding>
    </data>
  </capabilities>
</app:application>