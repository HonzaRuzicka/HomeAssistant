#define SN "Ampermetr-Termistor10k-Relay"
#define SV "0.2-52"
// Enable debug prints to serial monitor
//#define MY_DEBUG
// Enable and select radio type attached 
#define MY_RADIO_RF24
// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 
//Define Termistor10k
int ThermistorPin = 7;
int Vo;
float R1 = 5200;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

//Definice AmperMetr
byte adcsra_save;
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

//Definice Relay
#define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_LED 5
#define NUMBER_OF_RELAYS 1 // Total number of attached relays
#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay
bool initialValueSent = false;

#define SENSOR_TEMP_OFFSET 0     // used for temperature data and heat index computation
#define CHILD_ID_TEMP 4
#define CHILD_ID_ALERT 6

uint32_t delayMS;

//Definice odpovědí
MyMessage msgAmp(CHILD_ID_AMP, V_WATT);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msg1(1, V_LIGHT);
MyMessage msg(CHILD_ID_ALERT, V_TRIPPED);

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
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_ALERT, S_DOOR, "Alert");
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
//nejprve si uložím status nastavení ADCSRA (níže ho přenastavuji, tak abych se mohl vrátit zpět)
if (!adcsra_save)
{
  adcsra_save = ADCSRA;
}
//Termistor10k////////////////////////////////////
  uint8_t s;
  float average;
  int samplingrate = 10;
  int samples = 0;
  // take voltage readings from the voltage divider
  for (s = 0; s < 10; s++) {
    samples += analogRead(ThermistorPin);
    delay(20);
  }
  average = 0;
  average = samples / samplingrate;
  Vo = average;
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  
  if(T > 75){
    digitalWrite(RELAY_PIN, RELAY_OFF);
    digitalWrite(RELAY_LED, RELAY_ON);
    send(msg.set(1)); //Zapnu ALERT
  }
  if (T < 65){
    send(msg.set(0)); //Zapnu ALERT
  }
  
    #ifdef MY_DEBUG
    wait(100);
    Serial.print("Sending temperature: ");
    Serial.print(T);
    #endif    
    send(msgTemp.set(T + SENSOR_TEMP_OFFSET, 2));
    
//Ampermetr
 for (int i = 0; i < 1; i++){

 long lNoSamples=0;
 long lCurrentSumSQ = 0;
 long lCurrentSum=0;

  // set-up ADC - Zde přenastaveuji ADCSRA
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
  
  // stop sampling - vracím nastavení ADCSRA na původní úroveň
  ADCSRA = adcsra_save;
  
  if (lNoSamples>0)  // if no samples, micros did run over
  {  
    // correct quadradic current sum for offset: Sum((i(t)+o)^2) = Sum(i(t)^2) + 2*o*Sum(i(t)) + o^2*NoSamples
    // sum should be zero as we have sampled 5 cycles at 50Hz (or 6 at 60Hz)
    float fOffset = (float)lCurrentSum/lNoSamples;
    lCurrentSumSQ -= 2*fOffset*lCurrentSum + fOffset*fOffset*lNoSamples;
    if (lCurrentSumSQ<0) {lCurrentSumSQ=0;} // avoid NaN due to round-off effects
    
    float fCurrentRMS = sqrtf((float)lCurrentSumSQ/(float)lNoSamples) * gfACS712_Factor * gfLineVoltage / 1024;
    vysledek[i] = fCurrentRMS;//gfLineVoltage;
    
    #ifdef MY_DEBUG
    Serial.println(F("Now sending"));
    Serial.println(vysledek[i]);
    #endif  
    send(msgAmp.set(vysledek[i], 2));
    
    // correct offset for next round
    giADCOffset=(int)(giADCOffset+fOffset+0.5f);
  }
  wait(60000); // waiting for potential presentation requests
  }
}
void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.getType()==V_STATUS) {
        // Change relay state
        digitalWrite(message.getSensor()-1+RELAY_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
        digitalWrite(message.getSensor()-1+RELAY_LED, message.getBool()?RELAY_OFF:RELAY_ON);
        // Store state in eeprom
        saveState(message.getSensor(), message.getBool());
        // Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(message.getSensor());
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    }
}
