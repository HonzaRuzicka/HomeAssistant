int sensorTA12 = A0; // Analog input pin that sensor is attached to

float nVPP;   // Voltage measured across resistor
float nCurrThruResistorPP; // Peak Current Measured Through Resistor
float nCurrThruResistorRMS; // RMS current through Resistor
float nCurrentThruWire;     // Actual RMS current in Wire

void setup() 
 {
   Serial.begin(38400); 
   pinMode(sensorTA12, INPUT);
 }
 
 
 void loop() 
 {
   
   
   nVPP = getVPP();

   Serial.print("VPP: ");
   Serial.print(nVPP,3);
   Serial.print(" V         ");   
   /*
   Use Ohms law to calculate current across resistor
   and express in mA 
   */
   
   nCurrThruResistorPP = (nVPP/230.0) * 1000.0;
   
   /* 
   Use Formula for SINE wave to convert
   to RMS 
   */
   
   nCurrThruResistorRMS = nCurrThruResistorPP * 0.707;
   
   /* 
   Current Transformer Ratio is 1000:1...
   
   Therefore current through 200 ohm resistor
   is multiplied by 1000 to get input current
   */
   
   nCurrentThruWire = nCurrThruResistorRMS * 1000;
  
   Serial.print("Current: ");
   Serial.print(nCurrThruResistorRMS,3);
   Serial.println(" A ");   
 }


 /************************************ 
In order to calculate RMS current, we need to know
the peak to peak voltage measured at the output across the
200 Ohm Resistor

The following function takes one second worth of samples
and returns the peak value that is measured
 *************************************/
 
 
float getVPP()
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 10 Sec
   {
       readValue = analogRead(sensorTA12);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
   }
   
   // Convert the digital data to a voltage
   result = (maxValue * 5.0)/1024.0;
  
   return result;
 }
