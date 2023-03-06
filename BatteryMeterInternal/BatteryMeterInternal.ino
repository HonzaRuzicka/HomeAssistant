// Enable debug prints to serial monitor
//#define MY_DEBUG

#define SN "Battery Meter"
#define SV "1.2"

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

//Uncomment (and update) if you want to force Node Id
//#define MY_NODE_ID 100

#define MY_BAUD_RATE 38400

#define CHILD_ID_BATT 30
#define CHILD_ID_VOLTAGE 31

#define SENSOR_BATT_OFFSET 0 

#include <MySensors.h>
MyMessage msgVoltage(CHILD_ID_VOLTAGE,V_VOLTAGE);

uint32_t SLEEP_TIME = 60000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;
#define FULL_BATTERY 3.52 // 3V for 2xAA alkaline. Adjust if you use a different battery setup.

void setup()
{
}

void presentation()
{
// Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  // Register all sensors to gw (they will be created as child devices)
//  present(CHILD_ID_BATT, S_BATT, "Battery");
  wait(100);                                      //to check: is it needed
  present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Volt");
  wait(100);                                      
}

void loop()
{
  // get the battery Voltage
  long batteryMillivolts = hwCPUVoltage();
  int batteryPcnt = batteryMillivolts / FULL_BATTERY / 1000.0 * 100 + 0.5;
//#ifdef MY_DEBUG
  Serial.print("Battery voltage: ");
  Serial.print(batteryMillivolts / 1000.0);
  Serial.println("V");
  Serial.print("Battery percent: ");
  Serial.print(batteryPcnt);
  Serial.println(" %");
//#endif
  //if (oldBatteryPcnt != batteryPcnt) {
    sendBatteryLevel(batteryPcnt);
    send(msgVoltage.set(batteryMillivolts / 1000.0, 2));
    oldBatteryPcnt = batteryPcnt;
  //}
  sleep(SLEEP_TIME);
}
