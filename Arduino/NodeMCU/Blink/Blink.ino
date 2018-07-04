
#include "sensorBase.h"

#define LED_PIN D7

class MySensor : SensorBase
{
  private:
    
    bool blinkLow = false;
    bool timeoutSet;

  public:
    MySensor():SensorBase()
    {
    }

    void setup()
    {
      SensorBase::setup();  // Always call baseclass setup

      // initialize digital pin D7 as an output.
      pinMode(LED_PIN, OUTPUT);
    
      registerTimer(1000);

      timeoutSet = false;
    }

    void loop()
    {
      SensorBase::loop();   // Always call baseclass loop

      if (timeoutSet)
      {
        if (blinkLow)
        {
          digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
          debug_print("LOW");
        }
        else
        {
          digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
          debug_print("HIGH");
        }
        blinkLow = !blinkLow;
        timeoutSet = false;
      }
    }

    void onTimer() override 
    {
      timeoutSet = true;
    }

    void onMqqtMessage(char topic[], char payload[] ) override
    {

    }
};

MySensor mySensor;



// the setup function runs once when you press reset or power the board
void setup() {
  mySensor.setup();
}

// the loop function runs over and over again forever
void loop() {
  mySensor.loop();
 
  yield();
}

/* Timer functions */

