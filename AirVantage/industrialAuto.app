<?xml version="1.0" encoding="ISO-8859-1"?>
<app:application
        xmlns:app="http://www.sierrawireless.com/airvantage/application/1.0"
        type="com.test.mangOH.industrial.app"
        name="mangOH Industrial"
        revision="1.0.2">
 <capabilities>

  <communication>
   <protocol comm-id="IMEI" type="MQTT" />
  </communication>

  <data>
   <encoding type="MQTT">
    <asset default-label="Industrial" id="industrial">

     <variable default-label="RedLed" path="redled" type="boolean"/>
     <variable default-label="GreenLed" path="greenled" type="boolean"/>
     <setting default-label="Switch State" path="switch" type="boolean"/>
     <setting default-label="Overheat Switch" path="overheatSwitch" type="boolean"/>
     <variable default-label="Fan" path="fan" type="boolean"/>
     <command default-label="Power Command" id="power">
      <parameter default-label="State" default-value="true" id="state" type="boolean" />
     </command>
    </asset>
   </encoding>
  </data>
  </capabilities>
</app:application>
