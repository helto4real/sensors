# Blink sample
This is a sample of usage of the SensorBase baseclass.

- General base class for sensor that post and receives to MQTT and can debug through MQTT too
  
 - Send {"debuglevel": "[0-3]"} to the debug/sensor/[sensorname]/in topic to set debug level
   subscribe to topic debug/sensor/[sensorname]/out for debugmessages

 - Use defines.h to set wifi settings and mqtt settings

 This is early development but the aim for the base class is
 to make all plumbing for wifi, mqtt, debuglogging etc hidden 
 away from the implementation of the sensor code

 Use as you please....