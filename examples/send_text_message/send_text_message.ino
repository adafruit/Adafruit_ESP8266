
/* 
* Visit http://iot-https-relay.appspot.com/ and http://allaboutee.com for more info on this script. 
* Warning! Please be sure to understand the security issues of using this relay app and use at your own risk 
*  
* A sketch for making calls using IoT HTTPS Relay with Twilios API. Please note that there is a bug 
* where the Arduino keeps reseting resulting in many many calls. So be careful if you are using a 
* paid Twilio account.  If are able to fix this bug though please create a pull request, thanks! 
*/

/*------------------------------------------------------------------------
  Requires SoftwareSerial and an ESP8266 that's been
  flashed with recent 'AT' firmware operating at 9600 baud.  Only tested
  w/Adafruit-programmed modules: https://www.adafruit.com/product/2282

  The ESP8266 is a 3.3V device.  Safe operation with 5V devices (most
  Arduino boards) requires a logic-level shifter for TX and RX signals.
  ------------------------------------------------------------------------*/

#include <Adafruit_ESP8266.h>
#include <SoftwareSerial.h>

// Connect ESP TX pin to Arduino pin 3
#define ESP_RX   3
// Connect ESP RX pin to Arduino pin 4
#define ESP_TX   4
#define ESP_RST  8
SoftwareSerial softser(ESP_RX, ESP_TX);

// Must declare output stream before Adafruit_ESP8266 constructor; can be
// a SoftwareSerial stream, or Serial/Serial1/etc. for UART.
Adafruit_ESP8266 wifi(&softser, &Serial, ESP_RST);
// Must call begin() on the stream(s) before using Adafruit_ESP8266 object.

#define ESP_SSID "xxx" // Your network name here
#define ESP_PASS "xxxxxxx" // Your network password here

const char PHONE_FROM[] = "xxxxxxxxxxx"; // Your Twilio's phone number, including country code
const char PHONE_TO[] = "xxxxxxxxxxx"; // Any phone number including country code, BUT if you only have a free account this can only be your verified number
const char TWILIO_ACCOUNT_SID[] =  "xxxxxxxxxxxxxxx"; // Your Twilio's ACCOUNT SID
const char TWILIO_TOKEN[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // Your Twilios TOKEN
const char TEXT_MESSAGE_BODY[]  = "Hello+this+is+your+ESP8266"; // URL encoded text message.
char data[200]; // HTTP POST data string. Increase size as required.
               
#define HOST "iot-https-relay.appspot.com"
#define PORT 80

void setup() {
  char buffer[50];

  // This might work with other firmware versions (no guarantees)
  // by providing a string to ID the tail end of the boot message:
  
  // comment/replace this if you are using something other than v 0.9.2.4!
  wifi.setBootMarker(F("Version:0.9.2.4]\r\n\r\nready"));

  softser.begin(9600); // Soft serial connection to ESP8266
  Serial.begin(57600); while(!Serial); // UART serial debug

  Serial.println(F("Adafruit ESP8266 Phone Call"));

  // Test if module is ready
  Serial.print(F("Hard reset..."));
  if(!wifi.hardReset()) {
    Serial.println(F("no response from module."));
    for(;;);
  }
  Serial.println(F("OK."));

  Serial.print(F("Soft reset..."));
  if(!wifi.softReset()) {
    Serial.println(F("no response from module."));
    for(;;);
  }
  Serial.println(F("OK."));

  Serial.print(F("Checking firmware version..."));
  wifi.println(F("AT+GMR"));
  if(wifi.readLine(buffer, sizeof(buffer))) {
    Serial.println(buffer);
    wifi.find(); // Discard the 'OK' that follows
  } else {
    Serial.println(F("error"));
  }

  Serial.print(F("Connecting to WiFi..."));
  if(wifi.connectToAP(F(ESP_SSID), F(ESP_PASS))) {

    // IP addr check isn't part of library yet, but
    // we can manually request and place in a string.
    Serial.print(F("OK\nChecking IP addr..."));
    wifi.println(F("AT+CIFSR"));
    if(wifi.readLine(buffer, sizeof(buffer))) {
        Serial.println(buffer);
        wifi.find(); // Discard the 'OK' that follows

        Serial.print(F("Connecting to host..."));

        Serial.print("Connected..");
        wifi.println("AT+CIPMUX=0"); // configure for single connection, 
                                     //we should only be connected to one SMTP server
        wifi.find();
        wifi.closeTCP(); // close any open TCP connections
        wifi.find();

        if(wifi.connectTCP(F(HOST), PORT)) {
        Serial.print(F("OK\nRequesting page..."));


        strcat(data,"From=");
        strcat(data,PHONE_FROM);
        strcat(data,"&");

        strcat(data,"To=");
        strcat(data,PHONE_TO);
        strcat(data,"&");

        strcat(data,"sid=");
        strcat(data,TWILIO_ACCOUNT_SID);
        strcat(data,"&");

        strcat(data,"token=");
        strcat(data,TWILIO_TOKEN);
        strcat(data,"&");

        strcat(data,"Body=");
        strcat(data,TEXT_MESSAGE_BODY);
        
        wifi.httpPost("iot-https-relay.appspot.com","/twilio/Messages.json",data);

        wifi.closeTCP();
        while(1);
        } else { // TCP connect failed
        Serial.println(F("D'oh!"));
        }
        
    } else { // IP addr check failed
      Serial.println(F("error"));
    }
  } else { // WiFi connection failed
    Serial.println(F("FAIL"));
  }
}

void loop() {

}

