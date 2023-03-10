// Enable debug prints to serial monitor
//#define MY_DEBUG

#define SN "Battery Meter"
#define SV "1.5"
int inicializace = 0;

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


uint32_t SLEEP_TIME = 10000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;
float max_voltage = 3.48; // volty při 100% baterie
float min_voltage = 2.83; // volty při 0% baterie
bool nabijeni = false;

void before()
{
}

void setup()
{
}

#include <MySensors.h>
MyMessage msgVoltage(CHILD_ID_VOLTAGE, V_VOLTAGE);
MyMessage msgCharge(CHILD_ID_BATT, V_VAR1);
MyMessage msgBatt(CHILD_ID_BATT, V_VAR2);

void presentation()
{
  // Send the sketch version information to the gateway
  sendSketchInfo(SN, SV);
  //Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_BATT, S_CUSTOM, "Battery");
  wait(100);
  present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Volt");
  wait(100);
}

void loop()
{
  if (inicializace == 0) {
    Serial.print(SN);
    Serial.print(" ver.: ");
    Serial.println(SV);
    Serial.print("SleepTime: ");
    Serial.print(SLEEP_TIME / 1000);
    Serial.println(" sekund.");
    inicializace = 1;
  }

  // get the battery Voltage
  float batteryMillivolts = hwCPUVoltage();
  float voltynow = batteryMillivolts / 1000.0;
  int batteryPcnt = ((voltynow - min_voltage) / (max_voltage - min_voltage)) * 100;

  //pokud je při nabíjení má batterie více voltů nastavím to jako 100% - tohle by chtělo ještě nějak dodělat abych mohl nastavit že ne nabíjí.
  if (batteryPcnt > 100)  {
    batteryPcnt = 100;
    nabijeni = true;
  }
  else {
    nabijeni = false;
  }
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
    send(msgVoltage.set(batteryMillivolts / 1000.0, 2));
    send(msgCharge.set(nabijeni));
    send(msgBatt.set(batteryPcnt));
    oldBatteryPcnt = batteryPcnt;
  }
  sleep(SLEEP_TIME);
}
