
//#include <SoftwareSerial.h>

/*
  Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 2nd, 2016
  https://github.com/sparkfun/MAX30105_Breakout
  This is a demo to show the reading of heart rate or beats per minute (BPM) using
  a Penpheral Beat Amplitude (PBA) algorithm.
  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.
  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected
  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"

#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
int steps;
int counter = 0;
float temperature;
float temperatureF;
boolean state = 0;
unsigned long last_interrupt=0;

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
//#include <SparkFunESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
char ssid [30];

//-----------------------------------
// For the Dynamic SSID logic

// need ceil()
#include <math.h>

// Define a SSID Basename - note, spaces are evil
#define SSID_BASENAME  "IncredibleWearables"

// Max number of devices running at a time. This can be increased.
#define SSID_MAXNUMBER 64

// this makes the code easlier to understand...
#define BITS_PER_BYTE 8 

// Define a delay in seconds for our WiFi search
// This delay is used, along with a random number to present SSID collision
// when multiple boards are startup at the same time. 
#define WIFI_STARTUP_DELAY 6

// Variable that holds the SSID slot number
int ssidSlotNumber = -1;

// Delays for Slot Number output via LED (delay in ms)
#define SlotLongDelay   2000
#define SlotShortDelay  400

// How many times do we telegraph out our SSID number via the onboard LED
int nFlash = 2;

// this is always handy 
#define LED_PIN 5

// END Dynamic SSID -----------------------------------


//const char *password = "thereisnospoon";
String htmlText = ("<!DOCTYPE html><html><head><title>Incrediable Wearables</title></head><body style=background-color:#61c250>");

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {

	// If someone has connected to this device, we can stop flashing our SSID slot number.
	nFlash=0;

  htmlText.concat("<h1>You are connected to Incredible Wearables!</h1>");
  htmlText.concat("<br> <h2>Your heart rate is: ");
  htmlText.concat(String (int (beatAvg)));
  htmlText.concat(" </h2> ");
  htmlText.concat("<br>");
  htmlText.concat("<h2>Your temperature is:  ");
  htmlText.concat(String (temperatureF));
  htmlText.concat("</h2>");
  htmlText.concat("<br>");
  htmlText.concat("<h2>Number of steps taken=");
  htmlText.concat(String (counter));
  htmlText.concat("</h2>");




  //background-color: rgb(255,0,255);//background color
  //<svg height="210" width="500">
  //  <line x1="0" y1="0" x2="200" y2="200" style="stroke:rgb(255,0,0);stroke-width:2" />
  //</svg>
  //Begin line code
  //begin background color
  //htmlText.concat("div {background-size: 80px 60px;");
  //htmlText.concat("background-color: rgb(255,0,255); }");
  //end background color

  htmlText.concat("<table><tr> </tr> </table>");
  htmlText.concat("<svg height=");
  htmlText.concat('"');
  htmlText.concat("400");
  htmlText.concat("width=");
  htmlText.concat('"');
  htmlText.concat("800");
  htmlText.concat('"');
  htmlText.concat(">");


  htmlText.concat("<line x1=");
  htmlText.concat('"');
  htmlText.concat("0");
  htmlText.concat('"');

  htmlText.concat("y1=");
  htmlText.concat('"');
  htmlText.concat("200");
  htmlText.concat('"');


  htmlText.concat("x2=");
  htmlText.concat('"');
  htmlText.concat("800");
  htmlText.concat('"');

  htmlText.concat("y2=");
  htmlText.concat('"');
  htmlText.concat("200");
  htmlText.concat('"');

  htmlText.concat("style=");
  htmlText.concat('"');
  htmlText.concat("stroke:rgb(255,255,255);stroke-width:6");
  htmlText.concat('"');
  htmlText.concat("/>");
  //logo();
  htmlText.concat("</svg>");
  //end line code
  htmlText.concat("<br>");
  htmlText.concat("<br>");

  htmlText.concat("</body>");
  htmlText.concat("</html>");
  server.send(200, "text/html" , htmlText);



}

void setup() {

  delay(1000);
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(4, INPUT_PULLUP);//Tilt Sensor
  
  pinMode(12, INPUT_PULLUP);// sets pin for tilt switch counter

  Serial.println("Entering Setup");

  // setup/get our SSID - Use a dynamic method to find an ID that works in our WiFi environment
  if(!setupWiFiSSID()){
  	Serial.println("Error finding open WiFi SSID slot. Reverting to default");
  	strlcpy(ssid, SSID_BASENAME, sizeof(ssid));
  }
  Serial.print("SSID is: ");  Serial.println(String(ssid));

  //heartrate code
  Serial.println("Initializing...");



  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED


  //end heartrate code
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */

 /* New code here from Brad set wifi to AP access port- debug then */
  Serial.setDebugOutput(true); 
  /*Set debugging on - use serial monitor to see what is happening. */
  WiFi.mode(WIFI_AP);
  /*Set wifi mode to access port */
  
  WiFi.softAP(ssid);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
  attachInterrupt (digitalPinToInterrupt (4), switchPressed, CHANGE);
}

void loop() {


	// First, should we flash out our SSID number/slot
	if(nFlash > 0){
		telegraphSSIDSlotNumber();
		nFlash--;
	}

  //heartrate code
  long irValue = particleSensor.getIR();
  temperature = particleSensor.readTemperature();
  temperatureF = particleSensor.readTemperatureF(); //Because I am a bad global citizen

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      // Serial.print("BEATS: "); Serial.println(beatAvg);
      // Serial.print("TEMP: "); Serial.println(temperatureF);      

    }
  }

  if (irValue < 50000)
    Serial.print(" No finger?");

 
  server.handleClient();

  // Serial.println(counter);

}


// Interrupt Service Routine (ISR)
void switchPressed ()
{
  if (digitalRead (4) == LOW)//**change digitalRead (12) == LOW to digitalRead (4) == LOW
  {
      state = 1;
      digitalWrite (5, HIGH);
    if (digitalRead (4) == LOW&&(millis()-last_interrupt>400))//***change digitalRead (12) == LOW&&(millis()-last_interrupt>400)
    {                                                          //** to digitalRead (4) == LOW&&(millis()-last_interrupt>400)
      counter++;
      state = 0;
      last_interrupt=millis();
    }
  }
  
  else
  {
    digitalWrite (5, LOW);
    state = 0;
  }
} // end of switchPressed


///////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic SSID generation
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// findOpenSSIDSlot()
// 
// Purpose:
//    This function returns an integer that represents an "open" SSID slot for this device.
//    Where a "Slot" is just a number that is appended to the SSID basename. 
//
//    We limit our slots to 64 here, but can be adjusted via a #define at the top of this file.
//
// Return Value:
//     The number of the open SSID "slot". 
//
int findOpenSSIDSlot(void){

    // The plan -
    //      - Scan the network for all SSIDs
    //      - Loop through the found SSIDs, searching for IDs that start with our basename (see #define at top of file)
    //          - SSIDS have the format: <BASENAME><Slot number>  i.e. IncredibleWearables10 or IncredibleWearables33
    //      - If we find a used SSID, carve off the basename, and get the Slot number
    //      - Set the corresponsding bit in an bitmask to indicated a used slot
    //      - Once the scan is completed, find a non-set bit, this is an open slot we can use. 

    // Declare the bitmask - an unsigned char array with sufficent length to support the maxinum number
    // of clients declared at the top of this file (SSID_MAXNUMBER). 
    //
    // The bitmask is stack allocated -> size determined at compile time. Note the + 1 added to the size to 
    // take into account integer math, which removes any remainder. Yes, this wastes a byte if using a 
    // size that is divisable by BITS_PER_BYTE.

    unsigned char slotMask[ (SSID_MAXNUMBER/BITS_PER_BYTE) + 1] = {0};

    // Okay, lets scan for active WiFi access points.

    int nNet = WiFi.scanNetworks();  

    String strSSID;
    int slot;
    int iByte;

    // Loop over the found SSIDs, see if there are any more of our nodes out there

    for(int i=0; i < nNet; i++){

        strSSID = WiFi.SSID(i);  // get the SSID
        
        if(strSSID.startsWith(SSID_BASENAME)){

            // It's one of ours! get the slot number
            slot = strSSID.substring(strlen(SSID_BASENAME)).toInt(); 

            // Serial.print("USED SLOT: "); Serial.println(slot);

            // Set this position in our mask. Find the correct byte in our array to use
            iByte = slot/BITS_PER_BYTE;
            slotMask[iByte] |= 1 << (slot - iByte*BITS_PER_BYTE);


        }
    }
    // Okay, at this point our slotMask represents the state of our environment. 
    // Find an empty slot - i.e. an unset bit. 
    int theSlot = -1;

    // Note - we don't use slot 0, start at 1. The LED is flashed to indicate the slot number 
    //        to userspace. We can't flash the LED zero times (it's been tired)

    for(int i=1; i < SSID_MAXNUMBER; i++){

        iByte = i / BITS_PER_BYTE;

        if( ! ( slotMask[iByte] & (1 << (i - iByte*BITS_PER_BYTE) ) ) ){
            // The bit isn't set -> we have an empty slot -> we have a Winner
            theSlot = i;
            break;
        }
    }

    return theSlot;

}
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// setupWiFiSSID()
//
// Gets a open SSID slot for our current environment and builds an SSID.
//
// The SSID name and slot number globals (file scope) are set as part of this operation.
//
// The function returns true on success, false on failure

bool setupWiFiSSID(void){

    // First, we insert a short, random delay. 
    // Why? If 10 of these devices are truned on at the same instant, there's a chance 
    // they find the same open SSID slot. The helps pepper the environment with a little delta T.

    Serial.println("Starting delay");
    randomSeed(analogRead(0)); // Use the noise of pin 0 to seed the random generator
    delay(random(WIFI_STARTUP_DELAY * 1000)); // random number with a max value of WIFI_STARTUP_DELAY seconds


    // okay, find an open slot
    ssidSlotNumber = findOpenSSIDSlot();
    if(ssidSlotNumber == -1)
        return false;  // no joy in wifi land

    Serial.print("SSID SlotNumber: "); Serial.println(ssidSlotNumber);

    // Fill in the ssid string
    strlcpy(ssid, SSID_BASENAME, sizeof(ssid));
    strlcat(ssid, String(ssidSlotNumber).c_str(), sizeof(ssid));

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// telegraphSSIDSlotNumber()
//
// Purpose:
//    Use the onboard LED to communicate the WiFi slot number to userspace. 
// 
//    This just flashes the LED <slot number> times.
//
//    FUTURE DEVLOPER: 
//    If the full count method is an issue (larger numbers would take time to flash) 
//    you could blink out each decimal of the slot number. 
//
void telegraphSSIDSlotNumber(void){

    // Do we have a numbered slot? -1 => No, using default.
    //
    // If we are using the default, just turn on the pin and exit
	if(ssidSlotNumber < 1){
        digitalWrite(LED_PIN, LOW);
		return;
    }

    Serial.println("telegraphing SSID");

    // Throw a delay in here - so there is a pause between repeats.
    delay(SlotLongDelay);

    for(int i=0; i < ssidSlotNumber; i++){
        delay(SlotShortDelay);
        digitalWrite(LED_PIN, LOW);
        delay(SlotShortDelay);
        digitalWrite(LED_PIN, HIGH);   
    }

    digitalWrite(LED_PIN, HIGH);

}

