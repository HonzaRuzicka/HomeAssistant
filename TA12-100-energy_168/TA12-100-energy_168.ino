#define SN "Proud-168"
#define SV "0.1"
// Enable and select radio type attached 
#define MY_RADIO_RF24

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 

// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 6000; 

static const uint8_t FORCE_UPDATE_N_READS = 40;

#define CHILD_ID_PROUD 22

#define SENSOR_CURRENT 0 


// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)

uint32_t delayMS;
float lastTemp = 0;
float rozdilTemp;
int forceUpdateTemp; //hodnota pro poslání hodnoty když není změna víc jak 5 minut
int forceUpdateHum;
int forceUpdateLight;
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
  //to check: is it needed
  present(CHILD_ID_PROUD, S_CURRENT, "Proud");
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

  int value = digitalRead(LIGHTPIN);
  if(forceUpdateLight < FORCE_UPDATE_N_READS) {
    if (value != oldValue) {
     send(msgLight.set(value==HIGH ? 0 : 1));
     oldValue = value;
     forceUpdateLight = 0;
    }
  }
  else {
    send(msgLight.set(value==HIGH ? 0 : 1));
    oldValue = value;
    forceUpdateLight = 0;
  }
  

  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL);
  forceUpdateTemp++;
  forceUpdateHum++;
  forceUpdateLight++;
}
