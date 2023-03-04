// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

uint32_t SLEEP_TIME = 10000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;
#define FULL_BATTERY 6 // 3V for 2xAA alkaline. Adjust if you use a different battery setup.

void setup()
{
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Battery Meter", "1.0");
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
  if (oldBatteryPcnt != batteryPcnt) {
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }
  sleep(SLEEP_TIME);
}
