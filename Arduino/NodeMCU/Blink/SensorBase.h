#ifndef SensorBase_h
#define SensorBase_h

#include "defines.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

extern "C" {
#include "user_interface.h"
}

/**************************************************************************************************
      _    ___   __   __    ____     ___    _____   _____  __        __     _      ____    _____ 
     | |  / _ \  \ \ / /   / ___|   / _ \  |  ___| |_   _| \ \      / /    / \    |  _ \  | ____|
  _  | | | | | |  \ V /    \___ \  | | | | | |_      | |    \ \ /\ / /    / _ \   | |_) | |  _|  
 | |_| | | |_| |   | |      ___) | | |_| | |  _|     | |     \ V  V /    / ___ \  |  _ <  | |___ 
  \___/   \___/    |_|     |____/   \___/  |_|       |_|      \_/\_/    /_/   \_\ |_| \_\ |_____|
  >< Brought to you by HELTO                                                                                               
 
 - General base class for sensor that post and receives to MQTT and can debug through MQTT too
  
 - Send {"debuglevel": "[0-3]"} to the debug/sensor/[sensorname]/in topic to set debug level
   subscribe to topic debug/sensor/[sensorname]/out for debugmessages

 - Use defines.h to set wifi settings and mqtt settings

 Following libraries you will need:
    * PubSubClient
    * ArduinoJSON

 -> Big thanks for BRUH automation for initial inpiration
**************************************************************************************************/


#define MQTT_MAX_PACKET_SIZE 512


// Debug topics
#define debug_sensor_topic "debug/sensor/"SENSORNAME
#define debug_sensor_topic_in debug_sensor_topic"/in"
#define debug_sensor_topic_out debug_sensor_topic"/out"

/*
    Debuglevel, you set it when logging and you can control it whats actually sent
*/
enum DebugLevel : int
{
    None = 0,
    Normal = 1,
    Information = 2,
    Verbose = 3
};


int OTAport = 8266;
const int JSON_BUFFER_SIZE = 300;

/* Base class to derive from for you sensor, use following template

class MySensor : SensorBase
{
  public:
    MySensor():SensorBase()
    {

    }

    void setup()
    {
       SensorBase::setup();  // Always call baseclass setup

       // Insert you setup code here
    }

    void loop()
    {
      SensorBase::loop();   // Always call baseclass loop
      // Insert your loop code here
      yield();
    }

    void onTimer() override 
    {
      // Insert code if you set a timer;
    }

    void onMqqtMessage(char topic[], char payload[] ) override
    {
      // Insert code that parses payload / topic here
    }
};
*/
class SensorBase
{
    private:
        
        WiFiClient _wifi_client;
        PubSubClient _client;
        
        os_timer_t _myTimer;                             // used when times set
        bool _is_wifi_setup = false;                     // make sure we dont setup wifi more that once
        DebugLevel _debugLevel = DebugLevel::Normal;     // Normal debuglevel is default
        StaticJsonBuffer<JSON_BUFFER_SIZE> _jsonBuffer;  // Used to parse and create Json

 
        /*
            Restarts program from beginning but does not reset the peripherals and registers
        */
        void _resetDevice() // 
        {
            Serial.print("Resetting...");
            ESP.reset(); 
        }

        void _setupSerialLogging()
        {
            
            Serial.begin(115200);
            delay(10);

            Serial.println();
            Serial.println("Serial logging successfully setup");
        }

        /*
            Setup wifi 
        */
        void _setupWifi() 
        {
            if (_is_wifi_setup)
                return; // Make sure we setup only once
                
            delay(10);
            Serial.println();
            Serial.print("Connecting to ");
            Serial.println(wifi_ssid);

            WiFi.mode(WIFI_STA);
            WiFi.begin(wifi_ssid, wifi_password);

            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }

            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());

            _is_wifi_setup = true;
        }

        /*
            Setup OTA feature (NOT BEEN TESTED YET!!!!!)
        */
        void _setupOTA()
        {
            
            ArduinoOTA.setPort(OTAport);
            ArduinoOTA.setHostname(SENSORNAME);
            ArduinoOTA.setPassword((const char *)OTApassword);

            Serial.println("OTA is setup");

            ArduinoOTA.onStart([]() {
                Serial.println("Starting");
            });
            ArduinoOTA.onEnd([]() {
                Serial.println("\nEnd");
            });
            ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            });
            ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });
            ArduinoOTA.begin();     

            Serial.println("OTA successfully setup");   
    
        }

        /*
            Setup MQTT and callback 
        */
        void _setupMQTT() {
            _client.setServer(mqtt_server, mqtt_port);
            _client.setCallback([this] (char* topic, byte* payload, unsigned int length) { this->_mqttCallback(topic, payload, length); });
            _connectMQTT(); // Try to connect 
        }

        void _connectMQTT() {
            // Loop until we're reconnected
            while (!_client.connected()) 
            {
                Serial.print("Attempting MQTT connection...");
                // Attempt to connect
                if (_client.connect(SENSORNAME, mqtt_user, mqtt_password)) 
                {
                    Serial.println("connected");
                    _client.subscribe(debug_sensor_topic_in);
                    Serial.print("Subscribed to ");
                    Serial.println(debug_sensor_topic_in);
                } 
                else 
                {
                    Serial.print("failed, rc=");
                    Serial.print(_client.state());
                    Serial.println(" try again in 5 seconds");
                    // Wait 5 seconds before retrying
                    delay(5000);
                }
            }
        }
        
        /*
            Private callback function, calls "onTimer" for the sensor
        */
        static void _timerCallback(void *pArg) 
        {

            SensorBase *sensor = (SensorBase*) pArg;
            
            sensor->onTimer();
        } 
        
        /*
            Private callback function, calls "onMqqtMessage" for the sensor
        */
        void _mqttCallback(char* topic, byte* payload, unsigned int length) {
   
            char message[length + 1];

            memcpy(message, payload, length);
    
            message[length] = '\0';

            debug_printf(DebugLevel::Verbose, "Topic: [%s] \n Message: %s", topic, message);

            JsonObject* jsonPointer = _processJson(message);
            
            
            if (jsonPointer==NULL)
                return; // Can't process json message, its already logged. 

            JsonObject& json = *jsonPointer;                // Ik, ugly code :)
            if (strcmp(topic, debug_sensor_topic_in)==0)
            {
                // Debug topic
                if (json.containsKey("debuglevel"))
                {
                    _debugLevel = (DebugLevel) (int) json["debuglevel"];
                }
            }
            else
            {
                onMqqtMessage(topic, message);
            }
        }

        /*
            Returns a json object if parsed ok else NULL
        */
        JsonObject* _processJson(char* message) 
        {
            JsonObject& root = _jsonBuffer.parseObject(message);

            if (!root.success()) {
                debug_printf(DebugLevel::Information, "Fail to process json: \n %s", message);
                return NULL;
            }
            return &root;
        }

    protected:
        
        /*
            override in your sensor class to get callbacks when timer set
        */
        virtual void onTimer() {};

        /*
            override in your sensor class to get callbacks when mqtt arrives
        */
        virtual void onMqqtMessage(char topic[], char payload[] ) {};
       
        void setDebugLevel(DebugLevel level)
        {
            _debugLevel = level;
        }
        
        /*
            printf debug to serial and mqtt
        */
        void debug_printf(const char *format, ...) 
        {
            if((int) DebugLevel::Normal > (int)_debugLevel)
                return;             // Should not debug it
            va_list arg;
            va_start(arg, format);
            char temp[64];
            char* buffer = temp;
            size_t len = vsnprintf(temp, sizeof(temp), format, arg);
            va_end(arg);
            if (len > sizeof(temp) - 1) {
                buffer = new char[len + 1];
                if (!buffer) {
                    return ;
                }
                va_start(arg, format);
                vsnprintf(buffer, len + 1, format, arg);
                va_end(arg);
            }
            Serial.write((const uint8_t*) buffer, len);
            debug_print_mqtt(buffer);
        }

        //Failed to findout how to pass the "..." so I duped the code
        void debug_printf(DebugLevel debugLevel, const char *format, ...)
        {

            if((int)debugLevel > (int)_debugLevel)
                return;             

            va_list arg;                
            va_start(arg, format);
            char temp[64];
            char* buffer = temp;
            size_t len = vsnprintf(temp, sizeof(temp), format, arg);
            va_end(arg);
            if (len > sizeof(temp) - 1) {
                buffer = new char[len + 1];
                if (!buffer) {
                    return ;
                }
                va_start(arg, format);
                vsnprintf(buffer, len + 1, format, arg);
                va_end(arg);
            }
            Serial.write((const uint8_t*) buffer, len);
            debug_print_mqtt(buffer);
        }    

        void debug_print(const char s[])
        {
            debug_print(DebugLevel::Normal, s);
        }

        /*
            print debug to serial and mqtt
        */
        void debug_print(DebugLevel debugLevel, const char s[])
        {
            if((int)debugLevel > (int)_debugLevel)
                return;             // Should not debug it
            
            Serial.println(s);
            debug_print_mqtt(s);
        } 

        /*
            Default sensor setup, serial, wifi, OTA and MQTT yaya
        */
        void setup()
        {   _setupSerialLogging();
            _setupWifi();
            _setupOTA();
            _setupMQTT();

            debug_print(DebugLevel::Information, "SensorBase SETUP DONE!");
        }
        
        /*
            Publishes the debug message to MQTT
        */ 
        void debug_print_mqtt(const char * message)
        {
            _client.publish(debug_sensor_topic_out, message, true);
        }

        /*  
            Use to register a timer in ms 
        */
        void registerTimer(short time)
        {
            os_timer_setfn(&_myTimer, (os_timer_func_t *) &SensorBase::_timerCallback, this);
            os_timer_arm(&_myTimer, time, true);
        }

    public:
        
        SensorBase()
        {
            _client = PubSubClient(_wifi_client);
        }

        void loop()
        {
            if (!_client.connected()) 
            {
                // Do software reset if wifi down, safest way to recover
                _resetDevice();
            }
            _client.loop();
            ArduinoOTA.handle();

        }
};

#endif