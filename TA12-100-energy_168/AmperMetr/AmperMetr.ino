#define SN "Ampermetr"
#define SV "0.2-31"
// Enable and select radio type attached 
#define MY_RADIO_RF24

float gfLineVoltage = 230.0f;               // typical effective Voltage in Germany
float gfACS712_Factor = 27.03f;              // use 50.0f for 20A version, 75.76f for 30A version; 27.03f for 5A version, 29.30f for Grove
unsigned long gulSamplePeriod_us = 10000000;  // 100ms is 5 cycles at 50Hz and 6 cycles at 60Hz
int giADCOffset = 512; // initial digital zero of the arduino input from ACS712
float vysledek[3];

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 101

#define MY_BAUD_RATE 38400 

// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 6000; 

static const uint8_t FORCE_UPDATE_N_READS = 40;

#define CHILD_ID_AMP 31
#define SENSOR_S_POWER 0

// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)

MyMessage msgAmp(CHILD_ID_AMP, V_WATT);

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_AMP, S_POWER, "Sensor");
}

void setup()
{ 
//  delayMS = 1000;  
}

void loop()      
{   
//  delay(delayMS);

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

    send(msgAmp.set(vysledek[i] + S_POWER, 2));
    
    // correct offset for next round
    giADCOffset=(int)(giADCOffset+fOffset+0.5f);
  }
  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL);
}
}
