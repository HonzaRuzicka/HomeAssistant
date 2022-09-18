#define SN "Ampermetr-DHT-Relay"
#define SV "0.1-51"
// Enable debug prints to serial monitor
#define MY_DEBUG
// Enable and select radio type attached 
#define MY_RADIO_RF24
// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 

//Definice AmperMetr
float gfLineVoltage = 230.0f;               // typical effective Voltage in Germany
float gfACS712_Factor = 50.0f;              // use 50.0f for 20A version, 75.76f for 30A version; 27.03f for 5A version, 29.30f for Grove
unsigned long gulSamplePeriod_us = 10000000;  // 100ms is 5 cycles at 50Hz and 6 cycles at 60Hz
int giADCOffset = 512; // initial digital zero of the arduino input from ACS712
float vysledek[3];
// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 6000; 
static const uint8_t FORCE_UPDATE_N_READS = 40;
#define CHILD_ID_AMP 5
#define SENSOR_S_POWER 2

// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)
#include <Adafruit_Sensor.h> // Official "Adafruit Unified Sensor" by Adafruit (tested on version 1.1.1)
#include <DHT_U.h> // Official *DHT Sensor library* by Adafruit (tested on version 1.3.8) 

//Definice Relay
#define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 1 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
bool initialValueSent = false;

//Definice DHT
#define DHTTYPE DHT11
#define DHTDATAPIN 3
// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const uint64_t UPDATE_INTERVAL_DHT = 60000;
// Force sending an update of the temperature after n sensor reads, so a controller showing the
// timestamp of the last update doesn't show something like 3 hours in the unlikely case, that
// the value didn't change since;
// i.e. the sensor would force sending an update every UPDATE_INTERVAL*FORCE_UPDATE_N_READS [ms]
static const uint8_t FORCE_UPDATE_N_READS_DHT = 10;
// Set this offset if the sensors have permanent small offsets to the real temperatures/humidity.
// In Celsius degrees or moisture percent
#define SENSOR_HUM_OFFSET 0       // used for temperature data and heat index computation
#define SENSOR_TEMP_OFFSET 0      // used for humidity data
#define CHILD_ID_HUM 3
#define CHILD_ID_TEMP 4
DHT_Unified dhtu(DHTDATAPIN, DHTTYPE);
uint32_t delayMS;
float lastTemp;
float lastHum;
uint8_t nNoUpdates = FORCE_UPDATE_N_READS_DHT; // send data on start-up 
bool metric = true;
float temperature;
float humidity;

//Definice odpovědí
MyMessage msgAmp(CHILD_ID_AMP, V_WATT);
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msg1(1, V_LIGHT);

void before()
{
    for (int sensor=1, pin=RELAY_PIN; sensor<=NUMBER_OF_RELAYS; sensor++, pin++) {
        // Then set relay pins in output mode
        pinMode(pin, OUTPUT);
        // Set relay to last known state (using eeprom storage)
        digitalWrite(pin, loadState(sensor)?RELAY_ON:RELAY_OFF);
    }
}

void setup()
{ 
  dhtu.begin();
  sensor_t sensor;
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;  
}

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_AMP, S_POWER, "Sensor");
  for (int sensor=1, pin=RELAY_PIN; sensor<=NUMBER_OF_RELAYS; sensor++, pin++) {
        // Register all sensors to gw (they will be created as child devices)
        present(sensor, S_LIGHT, "Vypinac");
    }
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  metric = getControllerConfig().isMetric;
}


void loop()      
{ 
//Inicializace Realy pokud ještě neproběhla
if (!initialValueSent) {
    Serial.println("Sending initial value");
    send(msg1.set(loadState(1)?RELAY_OFF:RELAY_ON),true);
    wait(1000);
    Serial.println("Sending initial value: Completed");
    wait(5000);
    initialValueSent = true;
  }

//Ampermetr
  for (int i = 0; i < 1; i++){

 long lNoSamples=0;
 long lCurrentSumSQ = 0;
 long lCurrentSum=0;

  // set-up ADC
  ADCSRA = 0x87;  // turn on adc, adc-freq = 1/128 of CPU ; keep in min: adc converseion takes 13 ADC clock cycles
  
  if (i == 0)
  { ADMUX = 0X40;}  // internal 5V reference // PORT-40=A0 41=A1 42=A2, .....
  /*else if(i == 1)
  { ADMUX = 0X41;}
  else if(i == 2)
  { ADMUX = 0X42;}
  else if(i == 3)
  { ADMUX = 0X43;}*/

  // 1st sample is slower due to datasheet - so we spoil it
  ADCSRA |= (1 << ADSC);
  while (!(ADCSRA & 0x10));
  
  // sample loop - with inital parameters, we will get approx 800 values in 100ms
  unsigned long ulEndMicros = micros()+gulSamplePeriod_us;
  while(micros()<ulEndMicros)
  {
    // start sampling and wait for result
    ADCSRA |= (1 << ADSC);
    while (!(ADCSRA & 0x10));
    
    // make sure that we read ADCL 1st
    long lValue = ADCL; 
    lValue += (ADCH << 8);
    lValue -= giADCOffset;

    lCurrentSum += lValue;
    lCurrentSumSQ += lValue*lValue;
    lNoSamples++;
  }
  
  // stop sampling
  ADCSRA = 0x00;

  if (lNoSamples>0)  // if no samples, micros did run over
  {  
    // correct quadradic current sum for offset: Sum((i(t)+o)^2) = Sum(i(t)^2) + 2*o*Sum(i(t)) + o^2*NoSamples
    // sum should be zero as we have sampled 5 cycles at 50Hz (or 6 at 60Hz)
    float fOffset = (float)lCurrentSum/lNoSamples;
    lCurrentSumSQ -= 2*fOffset*lCurrentSum + fOffset*fOffset*lNoSamples;
    if (lCurrentSumSQ<0) {lCurrentSumSQ=0;} // avoid NaN due to round-off effects
    
    float fCurrentRMS = sqrtf((float)lCurrentSumSQ/(float)lNoSamples) * gfACS712_Factor * gfLineVoltage / 1024;
    vysledek[i] = fCurrentRMS;//gfLineVoltage;
    
    Serial.println(F("Now sending"));
    Serial.println(vysledek[i]);

    Serial.println(F("S_POWER"));
    Serial.println(S_POWER);

    send(msgAmp.set(vysledek[i], 2));
    
    // correct offset for next round
    giADCOffset=(int)(giADCOffset+fOffset+0.5f);
  }
  wait(300); // waiting for potential presentation requests
  //sleep(UPDATE_INTERVAL);
}
//DHT////////////////////////////////////
  sensors_event_t event;  
  // Get temperature event and use its value.
  dhtu.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else {
    temperature = event.temperature;
    #ifdef MY_DEBUG
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");
    #endif
  }

  // Get humidity event and use its value.
  dhtu.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity!");
  }
  else {
    humidity = event.relative_humidity;
    #ifdef MY_DEBUG
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    #endif
  }

if (fabs(humidity - lastHum)>=0.05 || fabs(temperature - lastTemp)>=0.05 || nNoUpdates >= FORCE_UPDATE_N_READS_DHT) {
    lastTemp = temperature;
    lastHum = humidity;
    nNoUpdates = 0; // Reset no updates counter
    
    #ifdef MY_DEBUG
    wait(100);
    Serial.print("Sending temperature: ");
    Serial.print(temperature);
    #endif    
    send(msgTemp.set(temperature + SENSOR_TEMP_OFFSET, 2));

    #ifdef MY_DEBUG
    wait(100);
    Serial.print("Sending humidity: ");
    Serial.print(humidity);
    #endif    
    send(msgHum.set(humidity + SENSOR_HUM_OFFSET, 2));

  }

  nNoUpdates++;
  sleep(UPDATE_INTERVAL_DHT);   
}
void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.getType()==V_STATUS) {
        // Change relay state
        digitalWrite(message.getSensor()-1+RELAY_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
        // Store state in eeprom
        saveState(message.getSensor(), message.getBool());
        // Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(message.getSensor());
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    }
}
