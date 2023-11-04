//*********************************EPS32************************
#include <WiFi.h>
#include <PubSubClient.h>

#define BUILTIN_LED 2
#define Float_Switch 19

// Update these with values suitable for your network.
const char* ssid = "Wifi name";               //Enter your Wifi name here 
const char* password = "Wiffi Password";      //Enter your Wifi Password

//Enter your mqtt server configurations
const char* mqtt_server = "192.....";    //Enter Your mqttServer address
const int mqttPort = 1883;       //Port number
const char* mqttUser = "mqtt_username"; //User
const char* mqttPassword = "mqtt_Password"; //Password
//const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);

// Serial receiving
bool newData = false;
const byte numChars = 100;
char receivedChars[numChars];

// Timing
unsigned long currentMillis;
const unsigned long ECPeriod = 1000;
unsigned long ECtime = millis() ;
unsigned long Floattime;
const unsigned long FloattimePeriod = 1000;
unsigned long temperatureMillis;
const unsigned long temperaturePeriod = 5000;


//Button to turn EC or pH calibration mode or switch back to main loop on UNO Board 
#define EC_SwitchButton 23
#define pH_SwitchButton 22

//String ECButtonpayload;

//Temperature sensors 
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 21
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float waterTemp;


void setup_wifi() 
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  char payloadStr[length + 1];              // Create a char array that's 1 byte longer than the incoming payload to copy it to and make room for the null terminator so it can be treated as string.
  memcpy(payloadStr, payload, length);
  //payloadStr[length + 1] = '\0';
  payloadStr[length] = '\0';


  /***************** CALLBACK: 4-Channel Relay Board (In Control Box) *****************/

  if (strcmp(topic, "control/relays") == 0) // Incoming message format will be <BOARD#>:<RELAY#>:<STATE>. STATE is "1" for on, "0" for off. Example payload: "1:1:0" = on board 1, turn relay 1 ON.
  {
    Serial.println(payloadStr);
    Serial2.print(">Relay:");               // Print this command to the Mega since it handles the relays.
    Serial2.print(payloadStr);
    Serial2.println('<');
  }

  if (strcmp(topic, "calibrate/caliEC") == 0)           //send command to Uno for EC calibration 
  {
    Serial.println(payloadStr);
    //Serial2.print(">a:");
    Serial2.println(payloadStr);
    //Serial2.println('<');
  }

  if (strcmp(topic, "calibrate/ECbutton") == 0)           ////Use the pin on/off to turn EC calition mode  
  {
    Serial.println(payloadStr);

    String Stringname = payloadStr; 
    Serial.print("String name: ");
    Serial.println(Stringname);

    if (Stringname == "High") 
    {
      digitalWrite(EC_SwitchButton, HIGH);
      client.publish("feedback/ECbutton","on");
      Serial.println("Turn HighEC");
    }
    else if (Stringname == "Low") 
    {
      digitalWrite(EC_SwitchButton, LOW);
      client.publish("feedback/ECbutton","off");
      Serial.println("Turn LOWEC");
    }
  }

  if (strcmp(topic, "calibrate/caliph") == 0)           //send command to Uno for pH calibration Value
  {
    Serial.println(payloadStr);
    Serial2.print(">P:");
    Serial2.println(payloadStr);
    Serial2.println('<');
  }

  if (strcmp(topic, "calibrate/pHbutton") == 0)          //Use the pin on/off to turn pH calition mode 
  {
    Serial.println(payloadStr);
   
    String Stringname = payloadStr; 
    Serial.print("String name: ");
    Serial.println(Stringname);

    if (Stringname == "High") 
    {
      digitalWrite(pH_SwitchButton, HIGH);
      client.publish("feedback/pHbutton","on");
      Serial.println("Turn HighpH");
    }
    else if (Stringname == "Low") 
    {
      digitalWrite(pH_SwitchButton, LOW);
      client.publish("feedback/pHbutton","off");
      Serial.println("Turn LOWpH");
    }
  }

  //send to Uno to adjust motor speed 
  if (strcmp(topic, "motorspeed") == 0)          
  {
    Serial.println(payloadStr);
    Serial2.print(">M:");
    Serial2.println(payloadStr);
    Serial2.println('<');
  }



}

void reconnect()
{
  byte mqttFailCount = 0;
  byte tooManyFailures = 10;
  // Loop until we're reconnected
  while (!client.connected())
  {
    if (mqttFailCount <= tooManyFailures)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect("EPS8266", mqttUser, mqttPassword))
      {
        Serial.println("connected");
        delay(1000);
        client.publish("feedback/general", "Garden controller connecting...");
        delay(1000);
        client.publish("feedback/general", "Garden controller connected.");
        digitalWrite(BUILTIN_LED, HIGH);

        client.subscribe("control/relays");
        client.subscribe("calibrate/ECbutton");
        client.subscribe("calibrate/caliEC");
        client.subscribe("calibrate/pHbutton");
        client.subscribe("calibrate/caliph");
        client.subscribe("motorspeed");
        client.subscribe("inTopic");
      }

      else
      {
        digitalWrite(BUILTIN_LED, LOW);
        mqttFailCount ++;
        Serial.print("Failed. Count = ");
        Serial.println(mqttFailCount);
        Serial.println("...trying again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    else
    {
      Serial.print(tooManyFailures);
      Serial.println(" MQTT failures in a row. Resetting WiFi connection.");
      WiFi.disconnect();
      delay(5000);
      setup_wifi();
      mqttFailCount = 0;
    }
  }
}

void setup() 
{
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  Serial2.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Float Switch sensor setup
  pinMode(Float_Switch, INPUT_PULLUP); 

  //Pin act like a button to switch EC or pH calibration mode or switch back to main loop on UNO board 
  pinMode(EC_SwitchButton, OUTPUT); 
  pinMode(pH_SwitchButton, OUTPUT);

  //temperature sensors 
  sensors.begin();
}

void loop() 
{
  currentMillis = millis();
  if (!client.connected()) 
  {
    reconnect();
  }

  recvWithStartEndMarkers();
  processSerialData();
  client.loop();

  if (currentMillis - Floattime >= FloattimePeriod)
  {
    FloatSwitch();
    Floattime = currentMillis;
  }
  else if (currentMillis - temperatureMillis >= temperaturePeriod )
  {
    Temp();
    temperatureMillis = currentMillis;
  }



}

void recvWithStartEndMarkers()                                       // Check for serial data from ESP32. Thanks to Robin2 for the serial input basics thread on the Arduino forums.
{
  static bool recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '>';
  char endMarker = '<';
  char rc;

  while (Serial2.available() > 0 && newData == false)
  {
    rc = Serial2.read();

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
    Serial.println(receivedChars);                    //debug. see what message receive
    client.publish("feedback/debug", receivedChars);
    char commandChar = receivedChars[0];
    switch (commandChar)
    {
      case 'R':                                                      // Relay feedback. Message format is "Relay FB:<BOARD#>:<RELAY#>:<STATUS>". Example: "Relay FB:0:4:1"
      {
        int boardNumber;
        int relayNumber;
        int relayPower;
        char* strtokIndx;  
        char buff[18];
      
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment which is the 'R'
        strtokIndx = strtok(NULL, ":");                            // Get the board number
        boardNumber = atoi(strtokIndx);
        strtokIndx = strtok(NULL, ":");                            // Get the relay number
        relayNumber = atoi(strtokIndx);  
        strtokIndx = strtok(NULL, ":");                            // Get the relay power state
        relayPower = atoi(strtokIndx);
        
        
        sprintf(buff, "%d:%d:%d", boardNumber, relayNumber, relayPower);
        client.publish("feedback/relays",buff);
       
        Serial.println(buff);
        break;
      }  

      case 'E':                                                    //receive EC value from EC sensors
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get EC value 
        client.publish("feedback/EC", strtokIndx);                  
        break;
      }
     
      case '*':                                                    //Calibration feedback after enter EC calibration mode
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, "*");                   // Get calibration state 
        //strtokIndx = strtok(NULL, "*");                        
        client.publish("feedback/calibration", strtokIndx);        
        //delay(1000);        
        break;
      }

      case 'W':                                                    //receive Water Temperture value from temperature sensors
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get EC value 
        client.publish("feedback/waterTemp", strtokIndx);                  
        break;
      }

      case 'F':                                                    //receive Flow value from UNO
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get flowRate value 
        client.publish("feedback/flowRate", strtokIndx);                  
        break;
      }

      case 'P':                                                    //receive PH value from PH sensors
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get PH value 
        client.publish("feedback/PH", strtokIndx);                  
        break;
      }

      case 'p':                                                    //receive PH Calibration value when switch to calibration mode 
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get PH value 
        client.publish("feedback/PHCalibration", strtokIndx);                  
        break;
      }

      case 'm':                                                    //receive motor speed feedback  
      {
        char* strtokIndx;
        strtokIndx = strtok(receivedChars, ":");                   // Skip the first segment 
        strtokIndx = strtok(NULL, ":");                            // Get PH value 
        client.publish("feedback/motorspeed", strtokIndx);                  
        break;
      }

    }
    newData = false;
  }
}


void FloatSwitch()                //Water Level 
{
    if(digitalRead(Float_Switch) == HIGH)
    {
      //Serial.println(">LevelWater: High<");
      client.publish("feedback/waterLevel", "High");
    }
    else
    {
      //Serial.println(">LevelWater: Low<");
      client.publish("feedback/waterLevel", "Low");
    }
}

void Temp()
{
    sensors.requestTemperatures(); // Send the command to get temperature readings 
    waterTemp = sensors.getTempCByIndex(0);

    char floatTemp[5];
    dtostrf(waterTemp, 1, 2, floatTemp); //first convert float to str
    char buff1[50];
    sprintf(buff1, ">T: %s  <", floatTemp); 
    Serial.println(buff1);
    Serial2.println(buff1);
}


