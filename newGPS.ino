/*
  SIM GPS Transmitter
  Simple project which logs data from GPS module (NEO 6M) into a web service using data HTTP GET requests through SIM800L module.
  Location is sent for each interval given as configuration variable 'frequency'.
  Connecting modules:
  Pin3 -> GPS-module-RX
  Pin4 -> GPS-module-TX
  Pin5 -> SIM-module-TX
  Pin6 -> SIM-module-RX

  Dependency(TinyGPS++ library): http://arduiniana.org/libraries/tinygpsplus/

  created   Jul 2017
  by CheapskateProjects
  ---------------------------
  The MIT License (MIT)
  Copyright (c) 2017 CheapskateProjects
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <TinyGPS++.h>

int counter = 0;
String readString;
int ledPin = 13;
String compare = String("+HTTPACTION: 0,200,810");
int _timeout;
String _buffer;
String number = "+621294400713";

// Config (Use APN corresponding to your service providers configs)
static String apn = "telkomsel";
//static String loggingPassword="qwerty";
//static String serverIP="1.2.3.4";

// Pins where GPS and SIM modules are connected
//static const int SimRXPin = 5, SimTXPin = 6;
//static const int GPSRXPin = 4, GPSTXPin = 3;
static const int ErrorPin = 10, SimConnectionPin = 12;

// Used baud rates (define based on used modules)
static const uint32_t SimBaudrate = 115200;
static const uint32_t GPSBaud = 9600;
static const uint32_t SerialBaudrate = 115200;

// How frequently we want to send the location (milliseconds)
static const unsigned long frequency = 15000;

// Maximum time to wait SIM module for response
static long maxResponseTime = 30000;

String responseString;
TinyGPSPlus gps;
unsigned long previous = 0;
unsigned long beltTime;
//SoftwareSerial sim_ss(SimRXPin, SimTXPin);
//SoftwareSerial gps_ss(GPSRXPin, GPSTXPin);

#define gps_ss Serial2
#define sim_ss Serial1

void setup()
{
  //  _buffer.reserve(50);
  // Initialize status pins
  pinMode(ErrorPin, OUTPUT);
  pinMode(SimConnectionPin, OUTPUT);
  digitalWrite(ErrorPin, LOW);
  digitalWrite(SimConnectionPin, LOW);

  /*
     Start serial communications. We can only listen to one ss at a time so changing that
     between sim and gps as needed
  */
  Serial.begin(SerialBaudrate);
  sim_ss.begin(SimBaudrate);
  gps_ss.begin(GPSBaud);
  //  sim_ss.listen();

  Serial.println("Waiting for init");
  // Wait few seconds so that module is able to take AT commands
  delay(10000);
  Serial.println("Init... waiting until module has connected to network");

  // Start AT communication. This sets auto baud and enables module to send data
  // sim_ss.println("AT");
  // Wait until module is connected and ready
  //  waitUntilResponse("SMS Ready");
  // blinkLed(SimConnectionPin);

  sim_ss.println("AT");
// Wait until module is connected and ready
// waitUntilResponse("SMS Ready");
  sim_ss.println("AT+CMGF=1");
  sim_ss.println("AT+SAPBR=3,1,\"APN\",\"telkomsel\"");  // change this for your cell provider
//waitUntilResponse("OK");
  sim_ss.println("AT+SAPBR=1,1");
// waitUntilResponse("OK");
  sim_ss.println("AT+HTTPINIT");
//waitUntilResponse("OK");
  sim_ss.println("AT+HTTPPARA=\"CID\",1");


  // Full mode
  //  sim_ss.println("AT+CFUN=1");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);

  //  // Set credentials (TODO username and password are not configurable from variables). This may work without CSTT and CIICR but sometimes it caused error without them even though APN is given by SAPBR
  //  sim_ss.write("AT+CSTT=\"");
  //  sim_ss.print(apn);
  //  sim_ss.write("\",\"\",\"\"\r\n");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);

  // Connect and get IP
  //  sim_ss.println("AT+CIICR");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);

  // Some more credentials
  //  sim_ss.println("AT+SAPBR=3,1,\"APN\",\"internet\"");
  //
  //  blinkLed(SimConnectionPin);
  //
  //  sim_ss.println("AT+SAPBR=3,1,\"USER\",\"\"");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);
  //
  //  sim_ss.println("AT+SAPBR=3,1,\"PWD\",\"\"");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);
  //
  //  sim_ss.println("AT+SAPBR=1,1");
  //  waitUntilResponse("OK");
  //  blinkLed(SimConnectionPin);
  //
  //  sim_ss.println("AT+HTTPINIT");
  //  waitUntilResponse("OK");
  //  digitalWrite(SimConnectionPin, HIGH);

  //  gps_ss.listen();
  previous = millis();
  Serial.println("starting loop!");
}

void blinkLed(int led)
{
  digitalWrite(led, HIGH);
  delay(20);
  digitalWrite(led, LOW);
}

/*
    Read from SIM serial until we get known response. TODO error handling!
 * */
void waitUntilResponse(String response)
{
  beltTime = millis();
  responseString = "";
  String totalResponse = "";
  while (responseString.indexOf(response) < 0 && millis() - beltTime < maxResponseTime)
  {

    readResponse();

    totalResponse = totalResponse + responseString;
    Serial.println(responseString);
    delay(300);
  }

  if (totalResponse.length() <= 0)
  {
    Serial.println("No response from the module. Check wiring, SIM-card and power!");
    digitalWrite(ErrorPin, HIGH);
    delay(30000);
    exit(0); // No way to recover
  }
  else if (responseString.indexOf(response) < 0)
  {
    Serial.println("Unexpected response from the module");
    Serial.println(totalResponse);
    digitalWrite(ErrorPin, HIGH);
    delay(30000);
    exit(0); // No way to recover
  }
}
void httpResponse(String response)
{
  beltTime = millis();
  responseString = "";
  String totalResponse = "";
  while (responseString.indexOf(response) < 0 && millis() - beltTime < maxResponseTime)
  {

    readResponse();

    totalResponse = totalResponse + responseString;
    responseString.trim();                                    // Trim whitespace 

    Serial.println("ThisIsResponseString :" +responseString); // Show ResponseString



    if (responseString == compare) {                          // Compare string "+HTTPACTION: 0,200,810" with real response from sim800
      counter = counter + 1;                                  // Counter value added  by 1 when condition satisfied
    }
    Serial.println(counter);                                  // Show counter value into Serial Monitor
  }

  if (totalResponse.length() <= 0)
  {
    Serial.println("No response from the module. Check wiring, SIM-card and power!");
    digitalWrite(ErrorPin, HIGH);
    delay(30000);
    exit(0); // No way to recover
  }
  else if (responseString.indexOf(response) < 0)
  {
    Serial.println("Unexpected response from the module");
    Serial.println(totalResponse);
    digitalWrite(ErrorPin, HIGH);
    delay(30000);
    exit(0); // No way to recover
  }
}

/*
   Read from serial until we get response line ending with line separator
 * */
void readResponse()
{
  responseString = "";
  while (responseString.length() <= 0 || !responseString.endsWith("\n"))
  {
    tryToRead();

    if (millis() - beltTime > maxResponseTime)
    {
      return;
    }
  }
}

/*
   If we have anything available on the serial, append it to response string
 * */
void tryToRead()
{
  while (sim_ss.available())
  {
    char c = sim_ss.read();                                      //gets one byte from serial buffer
    responseString += c;                                         //makes the string readString


  }
}

void loop()
{

//  if (counter ==5){
//    SendMessage();
//    delay(10000);
//    counter=0;
//    }
    
  
  // If we have data, decode and log the data
  while (gps_ss.available() > 0)
    if (gps.encode(gps_ss.read()))

      logInfo();

  // Test that we have had something from GPS module within first 10 seconds
  if (millis() - previous > 10000 && gps.charsProcessed() < 10)
  {
    Serial.println("GPS wiring error!");
    while (true);
  }
}

void logInfo()
{
  // Causes us to wait until we have satelite fix
  if (!gps.location.isValid())
  {
    Serial.println("Not a valid location. Waiting for satelite data.");
    blinkLed(ErrorPin);
    return;
  }

  // Only log once per frequency
  if (millis() - previous > frequency)
  {
    // sim_ss.listen();
    previous = millis();
    String url = "AT+HTTPPARA=\"URL\",\"http://";
    url += "trackkk.000webhostapp.com/?";
    url += "nama=";
    url += String(gps.location.lng(), 6);
    url += "&alamat=";
    url += String(gps.location.lat(), 6);
    url += "\"";
    sim_ss.println(url);
    waitUntilResponse("OK");
    digitalWrite(SimConnectionPin, LOW);
    sim_ss.println("AT+HTTPACTION=0");


    // DESIGNED FOR SERIAL READ


    //waitUntilResponse("+HTTPACTION:");
    httpResponse("+HTTPACTION:");                                // Wait for HTTP Response that begins with "+HTTPACTION:"

    // gps_ss.listen();
  }
}

void SendMessage()
{
  //Serial.println ("Sending Message");
      //Sets the GSM Module in Text Mode
  delay(1000);
  //Serial.println ("Set SMS Number");
  sim_ss.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
  delay(1000);
  String SMS = "Hello, how are you?";
  sim_ss.println(SMS);
  delay(100);
  sim_ss.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
  _buffer = _readSerial();
}

String _readSerial() {
  _timeout = 0;
  while  (!sim_ss.available() && _timeout < 12000  )
  {
    delay(13);
    _timeout++;
  }
  if (sim_ss.available()) {
    return sim_ss.readString();
  }
}

