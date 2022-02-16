#define SN "Topeni-Teplota"
#define SV "1.0"
// Enable and select radio type attached 
#define MY_RADIO_RF24

//#define MY_DEBUG

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 

// Set light PIN
#define LIGHTPIN 4

// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 6000; 

static const uint8_t FORCE_UPDATE_N_READS = 10;

#define CHILD_ID_HUM 20
#define CHILD_ID_TEMP 21
#define CHILD_ID_TOPENI 24

// Set this offset if the sensors have permanent small offsets to the real temperatures/humidity.
// In Celsius degrees or moisture percent
#define SENSOR_HUM_OFFSET 0       // used for temperature data and heat index computation
#define SENSOR_TEMP_OFFSET 0      // used for humidity data
#define SENSOR_LIGHT_OFFSET 0 


// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)
//#include <Adafruit_Sensor.h> // Official "Adafruit Unified Sensor" by Adafruit (tested on version 1.1.1)
#include "Wire.h"
#include "tinySHT2x.h"

tinySHT2x sht;

uint32_t delayMS;
float lastTemp = 0;
float lastHum = 0;
float rozdilTemp;
float rozdilHum;
uint8_t nNoUpdates = FORCE_UPDATE_N_READS; // send data on start-up 
bool metric = true;
float temperature;
float humidity;
int oldValue=-1;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgLight(CHILD_ID_TOPENI, V_LIGHT);

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_TOPENI, S_LIGHT, "Topeni");
}

void setup()
{
  sht.begin();
  pinMode(LIGHTPIN, INPUT); //nastavení pin senzoru 
  delayMS = 1000;  
}

void loop()      
{   
  delay(delayMS);
  
  temperature = sht.getTemperature();
  humidity = sht.getHumidity();
  
  /*Serial.println("--------------");
  Serial.println(sht.getTemperature());
  Serial.println(temperature);
  Serial.println(humidity);
  */
  rozdilTemp = temperature - lastTemp;
  if (rozdilTemp <0) {rozdilTemp * -1;}
  if (rozdilTemp > 1) {
    send(msgTemp.set(temperature + SENSOR_TEMP_OFFSET, 1));
    lastTemp = temperature;
  }

  rozdilHum = humidity - lastHum;
  if (rozdilHum <0) {rozdilHum * -1;}
  if (rozdilHum > 1) {
    send(msgHum.set(humidity + SENSOR_HUM_OFFSET, 1));
    lastHum = humidity;
  }

  // Get the update value
  int value = digitalRead(LIGHTPIN);
  Serial.println(value);
  if (value != oldValue) {
     send(msgLight.set(value==HIGH ? 0 : 1));
     oldValue = value;
  }

  nNoUpdates++;

  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL); 

}
