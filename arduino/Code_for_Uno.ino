//*****************************Uno***************************
#include <EEPROM.h>
#include "GravityTDS.h"

#define NUM_ELEMENTS(x)  (sizeof(x) / sizeof((x)[0])) // Use to calculate how many elements are in an array

//TDS(EC) sensors
#define TdsSensorPin A1
GravityTDS gravityTds;
float temperature ,tdsValue = 0;
float EC;
//switch on or off transistor 
int ecindex =0;
int ecB = 12;          //use transistor to turn EC black cable (ground) on/off
int ecR = 13;          //use transistor to turn EC Red cable (5Volt) on/off


float waterTemp;

// Serial receiving
bool newData = false;
const byte numChars = 100;
char receivedChars[numChars];

// Timing
unsigned long currentMillis;
unsigned long ECpHtime;
const unsigned long ECpHPeriod = 2000;
unsigned long temperatureMillis;
const unsigned long temperaturePeriod = 5000;
unsigned long Flowtime;
const unsigned long FlowtimePeriod = 1000;
unsigned long oldTime;          //Water flow rate time 


char* cmd;  

// Relays and motor driver L-298N
int relayPins[2][4]
{
  {4, 5, 6 },              // Motor drive L-298N: [0]= PH+ pump, [1]= PH- pump, [2]= EC pump
  {7, 8, 9, 10}                   // 4 Chan Relay Board
};

//Enable pin for all Motor drive Board L-298N. 
int enAll = 3;            //All Enable pin will connect to this pin. becasue we will set the all the motor same speed. 

//set motor speed (0-255). 0= 0 speed, 255= max speed 100%
int speed = 150; 

//Liquid flow rate sensor -DIYhacking.com Arvind Sanjeev
byte sensorInterrupt = digitalPinToInterrupt(2);  // 0 = digitalPinToInterrupt(2)
byte sensorPin       = 2;
float calibrationFactor = 4.5;   //The hall-effect flow sensor outputs approximately 4.5 pulses per second per litre/minute of flow.
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

//PH Sensors 
#define PH_PIN A0
int analog_PHvalue;
float Voltage;
float PH_slope;
float PH_Value = 0;
float PH4_volt = 3.1;       //Calibration. measure the probe in PH4 and record. If the number different than 3.1, then replace 3.1 with new measure number. 
float PH7_volt = 2.6;       //Calibration. measure the probe in PH7 and record. If the number different than 2.6, then replace 2.6 with new measure number. 


//EC Calibration 
int calibration = 0;


const int buttonPin = A5;  // Use ESP32 pin to send High or Low signal to UNO Pin (act like the pushbutton)
// variables will change:
int buttonState = 0;  // variable for reading the pushbutton status. "0" is LOW and "1" is HIGH
const int buttonPinA4 = A4;
int buttonStateA4 = 0;
 
void setup() 
{
    Serial.begin(115200);
    //sensors.begin();

  //Motor drive L-298N setup
    for (int i = 0; i <= 1; i++)
    {
      for (int j = 0; j <= NUM_ELEMENTS(relayPins[i]); j++)
      {
        pinMode(relayPins[i][j], OUTPUT);
        digitalWrite(relayPins[i][j], LOW);        //First power the Uno board on, the Motor is off. Motor turn on when is HIGH, and off when is LOW 
      }
    }
  //Relay setup
    for (int i = 1; i <= 2; i++)
    {
      for (int j = 0; j <= NUM_ELEMENTS(relayPins[i]); j++)
      {
        pinMode(relayPins[i][j], OUTPUT);
        digitalWrite(relayPins[i][j], HIGH);        //First power the Uno board on, the relay turn off. Relay turn on when is LOW, and off when is HIGH 
      }
    }
    digitalWrite(relayPins[1][1], HIGH);            //somehow the relay1 is not turn off from the for-loop above. so we need to write this code to make it turn off at being.

  //Enable pin for L-298N
    pinMode(enAll, OUTPUT);
  
  //flow rate sensor setsup
    pinMode(sensorPin, INPUT);
    digitalWrite(sensorPin, HIGH);

    pulseCount        = 0;
    flowRate          = 0.0;
    flowMilliLitres   = 0;
    totalMilliLitres  = 0;
    oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0. Configured to trigger on a FALLING state change (transition from HIGH state to LOW state)
   attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
 
  //PH sensor setup 
    pinMode (PH_PIN, INPUT);
  
  //TDS(EC) sensor setup
    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();  //initialization
    //use transistor to turn on/off EC cable 
    pinMode(ecB, OUTPUT);
    pinMode(ecR, OUTPUT);
    //turn transitor/ec sensor on when being power the microcotroller board 
    digitalWrite(ecB, HIGH);
    digitalWrite(ecR, HIGH);
}

void loop()
{
  //currentMillis = millis();
  buttonState = digitalRead(buttonPin);
  buttonStateA4 = digitalRead(buttonPinA4);
  

  if (calibration == 1)
  {  
    ECfunction();

    // // We can use Timer to get out or reset to "calibration = false"
    //   int index = 0; 
    //  currentMillis = millis();
    // if (currentMillis - ECpHtime >= ECpHPeriod)
    // {
    //   if (index <3)
    //   {
    //     ECpHtime = currentMillis;
    //     index ++;
    //   }
    //   else if (index ==3)
    //   {
    //     calibration = false;
    //     ECpHtime = currentMillis;
    //     index = 0;
    //   }
    // }

    // use ESP32 pin to act like a button, so we can get out or reset to "calibration = false"   
    if (buttonState == LOW) 
    {
      calibration = 0;
      Serial.println("Switch to calibration = 0");
    }
  }
  else if (calibration == 2) 
  {
    pHCalibration(); 
    if (buttonStateA4 == LOW) 
    {
      calibration = 0;
      Serial.println("Switch to calibration = 0");
      Serial.println(">p:exit<");
    } 
  
  }
  else if (calibration == 0)
  {
    Mloop();    
    if (buttonState == HIGH) 
    {
      calibration = 1;                                       //EC calibration 
      digitalWrite(ecB, HIGH);                               //Turn transitor on to connect EC black cable when it in calibration mode
      digitalWrite(ecR, HIGH);                               //Turn transitor on to connect EC Red cable when it in calibration mode
      Serial.println("Switch to calibration = 1");
    } 
    else if (buttonStateA4 == HIGH) 
    {
      calibration = 2;                                        //pH calibration 
      Serial.println("Switch to calibration = 2");
    }      
  }  
}

void Mloop() 
{
  //readTemperature();
  currentMillis = millis();

  recvWithStartEndMarkers();
  processSerialData();

  
  if (currentMillis - Flowtime >= FlowtimePeriod)
  {
    FlowVolume();
    Flowtime = currentMillis;
  }
  //Take EC or pH reading
  if (currentMillis - ECpHtime >= ECpHPeriod)
  {
    if (ecindex <4)            //take EC reading 4 time 
    {
      Serial.print(ecindex);    
      ECfunction();
      ecindex ++;
    }
    else if(ecindex == 4)       //turn off EC cable at index number 4
    {
      //Serial.println("                 ");
      Serial.print(ecindex);
      Serial.println(" turn EC off");
      digitalWrite(ecB, LOW);
      digitalWrite(ecR, LOW);
      ecindex ++;
    } 
    else if (ecindex == 5)      //then at index 5 take pH reading
    {
      Serial.print(ecindex);
      PHReading(); 
      ecindex ++;   
    }
    else if (ecindex == 6)       //turn EC cable back on at index 6 and reset index to 0
    {
      Serial.print(ecindex);
      Serial.println("turn EC on");   //turn EC on
      digitalWrite(ecB, HIGH);
      digitalWrite(ecR, HIGH);
      ecindex = 0;
    }
    ECpHtime = currentMillis;
  }
  if (currentMillis - temperatureMillis >= temperaturePeriod )
  {
    //Serial.println(waterTemp);
    Temp();
    temperatureMillis = currentMillis;
  }

}

void recvWithStartEndMarkers()                                      // Check for serial data from ESP32. Thanks to Robin2 for the serial input basics thread on the Arduino forums.
{
  static bool recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '>';
  char endMarker = '<';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          ndx = numChars - 1;
        }
      }
      else
      {
        receivedChars[ndx] = '\0';       
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }
    else if (rc == startMarker)
    {
      recvInProgress = true;
    }
  }
}

void processSerialData()
{
  if (newData == true)
  {
    Serial.print("received data: ");
    Serial.println(receivedChars);                    //deburg. see what message recieve
    char commandChar = receivedChars[0];
    switch (commandChar)
    {
      case 'R':                                                   // If message starts with "R", it's for relays. Message format is "Relay:<BOARD#>:<RELAY#>:<STATUS>". Example: "Relay:0:4:1"
      {
        int boardNumber;
        int relayNumber;
        int relayPower;
        char* strtokIndx;  
        //char buff[20];
    
        strtokIndx = strtok(receivedChars, ":");                    // Skip the first segment which is the 'R' character 
        strtokIndx = strtok(NULL, ":");                             // Get the board number
        boardNumber = atoi(strtokIndx);
        strtokIndx = strtok(NULL, ":");                             // Get the relay number
        relayNumber = atoi(strtokIndx);  
        strtokIndx = strtok(NULL, ":");                             // Get the relay power state
        relayPower = atoi(strtokIndx);
        
        //triggerRelay(boardNumber, relayNumber, relayPower);
        Serial.println(">  < ");                                   //clean memory before send to ESP32 board. This help EPS32 not get error "Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled."
        // sprintf(buff, ">Relay FB:%d:%d:%d<", boardNumber, relayNumber, relayPower);
        Serial.print(">Relay FB: ");
        Serial.print(boardNumber);
        Serial.print(":");
        Serial.print(relayNumber);
        Serial.print(":");
        Serial.print(relayPower);
        Serial.println("<");
        triggerRelay(boardNumber, relayNumber, relayPower);

        break;
      }
      
      case 'T':                                                   // "T" Receive temperature value from esp32
      {
        char buff[20];

        cmd = strtok(receivedChars, ":");                         //Skip the first segment which is the 'T' character
        cmd = strtok(NULL, ":");                                  //Get Value number        
        waterTemp = atof(cmd);                                    //convert String to float number 
        sprintf(buff, ">T:%s<", cmd);
        //Serial.println(buff);
        break;
      }

      case 'P':                                           // "P" Receive pH calibration value from esp32
      {
        //char* strtokIndx; 
        char buff[20];

        cmd = strtok(receivedChars, ":");                 //Skip the first segment which is the 'P' character
        cmd = strtok(NULL, ":");                          //Get Calibration value number when put in pH4 solution      
        PH4_volt = atof(cmd);
        cmd = strtok(NULL, ":");                          //Get Value number value number when put in pH7 solution
        PH7_volt = atof(cmd); 

        Serial.print(">p:");
        Serial.print(PH4_volt);
        Serial.print("/");
        Serial.print(PH7_volt);
        Serial.println("<");
        break;
      }

      case 'M':                                                   // "M" Receive Motor speed value from esp32
      {
        //char buff[20];

        cmd = strtok(receivedChars, ":");                         //Skip the first segment which is the 'M' character
        cmd = strtok(NULL, ":");                                  //Get Value number        
        speed = atoi(cmd);                                    //convert String to float number 
        Serial.print(">m:");
        Serial.print(speed);
        Serial.println("<");
        break;
      }
      
      
    }
    newData = false;
  }
}


void triggerRelay(int boardNumber, int relayNumber, int relayTrigger)
{
  Serial.println("< >");
  if (relayTrigger == 1)
  {
    digitalWrite(relayPins[boardNumber][relayNumber], HIGH); // Turn relay ON
    if (boardNumber == 0)   //This statement only for motor driver L-298N board
    {
      analogWrite(enAll, speed);     //set motor speed (0-255). 0= 0 speed, 255= max speed 100%
    }
  }
  else if (relayTrigger == 0)
  {
    digitalWrite(relayPins[boardNumber][relayNumber], LOW); // Turn relay OFF
  }
}

void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

void FlowVolume()
{
  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    unsigned int frac;
    
    /*// Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t"); 		  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");        
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 
    Serial.print("\t"); 		  // Print tab space
	  Serial.print(totalMilliLitres/1000);
	  Serial.print("L");*/
    
    char buff3[25];
    sprintf(buff3, ">FlowRate: %d :L/min <", int(flowRate)); 
    Serial.println(buff3);
    
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

void PHReading()
{
  int PH4 = 4;          //PH Buffer Solution liquid or power pack
  int PH7 = 7;          //PH Buffer Solution liquid or power pack
  //float PH4_volt = 3.1;       //Calibration. measure the probe in PH4 and record. If the number different than 3.1, then replace 3.1 with new measure number. 
  //float PH7_volt = 2.6;       //Calibration. measure the probe in PH7 and record. If the number different than 2.6, then replace 2.6 with new measure number. 

  analog_PHvalue = analogRead(PH_PIN);
  Voltage = analog_PHvalue * (5.0 / 1023.0) ;   //convert to voltage
  
  PH_slope = (PH7_volt-PH4_volt)/(PH7-PH4);  //find slope. m=(Y2-Y1)/(X2-X1);
  PH_Value = PH7 + ((Voltage - PH7_volt)/PH_slope);   //use linear equation "Y2-Y1 = m(X2-X1)" to find new PH value. 

  char floatPH_Value[5];
  dtostrf(PH_Value, 1, 2, floatPH_Value); //first convert float to str
  char buff4[25];
  sprintf(buff4, ">PHvalue: %s  <", floatPH_Value); 
  Serial.println(buff4);  
  Serial.println(PH4_volt);
  Serial.println(":");
  Serial.println(PH7_volt);
}

void ECfunction()
{
  gravityTds.setTemperature(waterTemp);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  // Serial.print(tdsValue,0);
  // Serial.println("ppm");
  EC = tdsValue;
  //EC = tdsValue*2;
  //EC/=1000; //convert us/cm to ms/cm
  //Serial.println(EC,2); //two decimal
  //Serial.println("ms/cm");

  char floatValueEC[5];
  dtostrf(EC, 1, 2, floatValueEC); //first convert float to str
  char buff1[25];
  sprintf(buff1, ">EC: %s :ms/cm <", floatValueEC); 
  Serial.println(buff1);
  delay(1000);
}

void Temp()
{
    //Receive water temperature from ESP32 
    // char floatTemp[5];
    // dtostrf(waterTemp, 1, 2, floatTemp); //first convert float to str
    // char buff1[50];
    // sprintf(buff1, ">WaterTemp: %s  <", floatTemp); 
    // Serial.println(buff1);
    Serial.print(">WaterTemp:");
    Serial.print(waterTemp);
    Serial.println("<");
}

void pHCalibration() 
{ 
  analog_PHvalue = analogRead(PH_PIN);
  Voltage = analog_PHvalue * (5.0 / 1023.0) ;   //convert to voltage
  
  Serial.print(">p: ");
  Serial.print(Voltage); 
  Serial.println(" <");
  delay(1000); 
}

