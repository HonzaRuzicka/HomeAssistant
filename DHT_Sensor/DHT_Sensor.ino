
/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
 * REVISION HISTORY
 * Version 1.0 (humidity sensor): Henrik EKblad
 * Version 1.1 - 2016-07-20 (humidity sensor): Converted to MySensors v2.0 and added various improvements - Torben Woltjen (mozzbozz)
 * Version 2.0 - 2018-09-25: Converted to DHTU Adafruit library - Tiberio Galletti
 * Version 2.1 - 2018-10-06: Clearer code and... if something changed... every sensor data are sent to gateway - Tiberio Galletti
 * Version 2.2 - 2018-12-27: Heat Index calculation included in sketch (based on Adafruit official library) - Tiberio Galletti
 * 
 * DESCRIPTION
 * This sketch provides an example of how to implement a humidity/temperature sensor using a DHT11/DHT21/DHT22. 
 * It inlcudes Heat Index *sensor*
 *  
 * For more information, please visit:
 * http://www.mysensors.org/build/TempHumFeel-DHT
 * 
 */
#define SN "TempHumFeel"
#define SV "2.2"

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached 
#define MY_RADIO_RF24
//#define MY_RADIO_RFM69
//#define MY_RS485

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 100

#define MY_BAUD_RATE 38400

// Uncomment the type of sensor in use:
#define DHTTYPE           DHT11     // DHT 11 
//#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)


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

#define CHILD_ID_HUM 60
#define CHILD_ID_TEMP 61
#define CHILD_ID_HEATINDEX 62

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

void presentation()  
{ 
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_HEATINDEX, S_TEMP, "Heat Index");
  metric = getControllerConfig().isMetric;
}

void setup()
{
  pinMode(DHTPOWERPIN, OUTPUT);
  digitalWrite(DHTPOWERPIN, HIGH);
  
  dhtu.begin();

  
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dhtu.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.print  ("Min Delay:   "); Serial.print(sensor.min_delay/1000); Serial.println(" ms");  
  Serial.println("------------------------------------");
  
  // Print humidity sensor details.
  dhtu.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.print  ("Min Delay:   "); Serial.print(sensor.min_delay/1000); Serial.println(" ms");  
  Serial.println("------------------------------------");
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;  
}

void loop()      
{  
  digitalWrite(DHTPOWERPIN, HIGH);   
  delay(delayMS+1000);
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
    nNoUpdates = 0; // Reset no updates counter
    
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

  }

  nNoUpdates++;

  // Sleep for a while to save energy
  digitalWrite(DHTPOWERPIN, LOW); 
  wait(300); // waiting for potential presentation requests
  sleep(UPDATE_INTERVAL); 

}
