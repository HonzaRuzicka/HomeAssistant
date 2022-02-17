#define SN "TopeniSwitch-Teplota-168"
#define SV "1.0"
// Enable and select radio type attached 
#define MY_RADIO_RF24

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 

// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 6000; 

static const uint8_t FORCE_UPDATE_N_READS = 40;

#define CHILD_ID_HUM 20
#define CHILD_ID_TEMP 21
#define CHILD_ID_TOPENI 22

// Set this offset if the sensors have permanent small offsets to the real temperatures/humidity.
// In Celsius degrees or moisture percent
#define SENSOR_HUM_OFFSET 0       // used for temperature data and heat index computation
#define SENSOR_TEMP_OFFSET 0      // used for humidity data

#define RELAY_PIN 3  // Arduino Digital I/O pin number
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)
#include "Wire.h"
#include "tinySHT2x.h"

tinySHT2x sht;

uint32_t delayMS;
float lastTemp = 0;
float rozdilTemp;
int forceUpdateTemp; //hodnota pro poslání hodnoty když není změna víc jak 5 minut
int forceUpdateHum;
float temperature;
float humidity;

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
  sht.begin(); //inicializace teploměru
  delayMS = 1000;
  //set relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, loadState(RELAY_PIN)?RELAY_ON:RELAY_OFF);  
}

void loop()      
{   
  delay(delayMS);
  
  temperature = sht.getTemperature();
  humidity = sht.getHumidity();
  
  if (forceUpdateTemp < FORCE_UPDATE_N_READS){
    rozdilTemp = temperature - lastTemp;
    if (rozdilTemp < 0) {rozdilTemp = rozdilTemp * -1;}
    if (rozdilTemp > 0.25) {
      send(msgTemp.set(temperature + SENSOR_TEMP_OFFSET, 1));
      lastTemp = temperature;
      forceUpdateTemp = 0;
    }
  }
  else {
    send(msgTemp.set(temperature + SENSOR_TEMP_OFFSET, 1));
      lastTemp = temperature;
      forceUpdateTemp = 0;
  }
  
  if (forceUpdateHum = FORCE_UPDATE_N_READS){
    send(msgHum.set(humidity + SENSOR_HUM_OFFSET, 1));
    forceUpdateHum = 0;
  }
  
  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL);
  forceUpdateTemp++;
  forceUpdateHum++;
}

void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.getType()==V_STATUS) {
        // Change relay state
        digitalWrite(RELAY_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
        // Store state in eeprom
        saveState(RELAY_PIN, message.getBool());
        /*// Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(RELAY_PIN);
        Serial.print(", New status: ");
        Serial.println(message.getBool());
        */
    }
}
