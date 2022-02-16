/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2019 Sensnology AB
* Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*******************************
*
* DESCRIPTION
* The ArduinoGateway prints data received from sensors on the serial link.
* The gateway accepts input on serial which will be sent out on radio network.
*
* The GW code is designed for Arduino Nano 328p / 16MHz
*
* Wire connections (OPTIONAL):
* - Inclusion button should be connected between digital pin 3 and GND
* - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
*
* LEDs (OPTIONAL):
* - To use the feature, uncomment any of the MY_DEFAULT_xx_LED_PINs
* - RX (green) - blink fast on radio message received. In inclusion mode will blink fast only on presentation received
* - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
* - ERR (red) - fast blink on error during transmission error or receive crc error
*
*/

// Enable debug prints to serial monitor
// #define MY_DEBUG


// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable serial gateway
#define MY_GATEWAY_SERIAL

// Define a lower baud rate for Arduinos running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Inverses the behavior of leds
#define MY_WITH_LEDS_BLINKING_INVERSE

// Flash leds on rx/tx/err
// Uncomment to override default HW configurations
#define MY_DEFAULT_ERR_LED_PIN 4  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  5  // the PCB, on board LED

#include <MySensors.h>

#define SN "TempHumFeel"
#define SV "2.2"

// Enable and select radio type attached 
#define MY_RADIO_RF24

//Uncomment (and update) if you want to force Node Id
#define MY_NODE_ID 0

#define MY_BAUD_RATE 38400

// Uncomment the type of sensor in use:
#define DHTTYPE           DHT11     // DHT 11 

// Set this to the pin you connected the DHT's data and power pins to; connect wires in coherent pins
#define DHTDATAPIN        3         
#define DHTPOWERPIN       8

// Sleep time between sensor updates (in milliseconds) to add to sensor delay (read from sensor data; typically: 1s)
static const uint64_t UPDATE_INTERVAL = 60000; 

// Force sending an update of the temperature after n sensor reads, so a controller showing the
// timestamp of the last update doesn't show something like 3 hours in the unlikely case, that
// the value didn't change since;
// i.e. the sensor would force sending an update every UPDATE_INTERVAL*FORCE_UPDATE_N_READS [ms]
static const uint8_t FORCE_UPDATE_N_READS = 10;

#define CHILD_ID_HUM 51
#define CHILD_ID_TEMP 52
#define CHILD_ID_HEATINDEX 53

// Set this offset if the sensors have permanent small offsets to the real temperatures/humidity.
// In Celsius degrees or moisture percent
#define SENSOR_HUM_OFFSET 0       // used for temperature data and heat index computation
#define SENSOR_TEMP_OFFSET 0      // used for humidity data
#define SENSOR_HEATINDEX_OFFSET 0   // used for heat index data


// used libraries: they have to be installed by Arduino IDE (menu path: tools - manage libraries)
#include <MySensors.h>  // *MySensors* by The MySensors Team (tested on version 2.3.2)
#include <Adafruit_Sensor.h> // Official "Adafruit Unified Sensor" by Adafruit (tested on version 1.1.1)
#include <DHT_U.h> // Official *DHT Sensor library* by Adafruit (tested on version 1.3.8) 

DHT_Unified dhtu(DHTDATAPIN, DHTTYPE);
// See guide for details on Adafruit sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

uint32_t delayMS;
float lastTemp;
float lastHum;
uint8_t nNoUpdates = FORCE_UPDATE_N_READS; // send data on start-up 
bool metric = true;
float temperature;
float humidity;
float heatindex;

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgHeatIndex(CHILD_ID_HEATINDEX, V_TEMP);



float computeHeatIndex(float temperature, float percentHumidity) {
  // Based on Adafruit DHT official library (https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp)
  // Using both Rothfusz and Steadman's equations
  // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml

  float hi;

  temperature = temperature + SENSOR_TEMP_OFFSET; //include TEMP_OFFSET in HeatIndex computation too
  temperature = 1.8*temperature+32; //convertion to *F

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 +
             2.04901523 * temperature +
            10.14333127 * percentHumidity +
            -0.22475541 * temperature*percentHumidity +
            -0.00683783 * pow(temperature, 2) +
            -0.05481717 * pow(percentHumidity, 2) +
             0.00122874 * pow(temperature, 2) * percentHumidity +
             0.00085282 * temperature*pow(percentHumidity, 2) +
            -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  hi = (hi-32)/1.8;
  return hi; //return Heat Index, in *C
}

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  //wait(100);                                      //to check: is it needed
  //present(CHILD_ID_HEATINDEX, S_TEMP, "Heat Index");
  metric = getControllerConfig().isMetric;
}


void setup()
{
  pinMode(DHTPOWERPIN, OUTPUT);
  digitalWrite(DHTPOWERPIN, HIGH);
//Serial.begin(9600); 
  // Initialize device.
  dhtu.begin();

  
  //Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dhtu.temperature().getSensor(&sensor);
  /*Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.print  ("Min Delay:   "); Serial.print(sensor.min_delay/1000); Serial.println(" ms");  
  Serial.println("------------------------------------");
  */
  // Print humidity sensor details.
  dhtu.humidity().getSensor(&sensor);
  /*Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.print  ("Min Delay:   "); Serial.print(sensor.min_delay/1000); Serial.println(" ms");  
  Serial.println("------------------------------------");
  */
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;  
}

void loop()      
{  
  digitalWrite(DHTPOWERPIN, HIGH);   
  delay(delayMS);
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

if (fabs(humidity - lastHum)>=0.05 || fabs(temperature - lastTemp)>=0.05 || nNoUpdates >= FORCE_UPDATE_N_READS) {
    lastTemp = temperature;
    lastHum = humidity;
    heatindex = computeHeatIndex(temperature,humidity); //computes Heat Index, in *C
    nNoUpdates = 0; // Reset no updates counter
    #ifdef MY_DEBUG
    Serial.print("Heat Index: ");
    Serial.print(heatindex);
    Serial.println(" *C");    
    #endif
    
    if (!metric) {
      temperature = 1.8*temperature+32; //convertion to *F
      heatindex = 1.8*heatindex+32; //convertion to *F
    }
    
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

    #ifdef MY_DEBUG
    wait(100);
    Serial.print("Sending HeatIndex: ");
    Serial.print(heatindex);
    #endif
    send(msgHeatIndex.set(heatindex + SENSOR_HEATINDEX_OFFSET, 2));

  }

  nNoUpdates++;

  // Sleep for a while to save energy
  digitalWrite(DHTPOWERPIN, LOW); 
  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL); 

}
