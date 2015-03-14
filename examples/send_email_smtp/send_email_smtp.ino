
/*------------------------------------------------------------------------
  Simple ESP8266 test.  Requires SoftwareSerial and an ESP8266 that's been
  flashed with recent 'AT' firmware operating at 9600 baud.  Only tested
  w/Adafruit-programmed modules: https://www.adafruit.com/product/2282

  The ESP8266 is a 3.3V device.  Safe operation with 5V devices (most
  Arduino boards) requires a logic-level shifter for TX and RX signals.
  ------------------------------------------------------------------------*/

#include <Adafruit_ESP8266_SMTP.h>
#include <SoftwareSerial.h>

#define ESP_RX   3
#define ESP_TX   4
#define ESP_RST  8
SoftwareSerial softser(ESP_RX, ESP_TX);

// Must declare output stream before Adafruit_ESP8266 constructor; can be
// a SoftwareSerial stream, or Serial/Serial1/etc. for UART.
Adafruit_ESP8266_SMTP wifi(&softser, &Serial, ESP_RST);
// Must call begin() on the stream(s) before using Adafruit_ESP8266 object.

#define ESP_SSID "ssid" // Your network name here
#define ESP_PASS "pass" // Your network password here

char EMAIL_FROM[] = "";
char EMAIL_PASSWORD[] =  "";
char EMAIL_TO[] = "";
char SUBJECT[]  = "";
String content = "";

#define HOST     "smtp.server.com"     // Host to contact

#define PORT     587                     // 80 = HTTP default port

int count = 0;

void setup() {
  char buffer[50];

  // This might work with other firmware versions (no guarantees)
  // by providing a string to ID the tail end of the boot message:
  
  // comment/replace this if you are using something other than v 0.9.2.4!
  wifi.setBootMarker(F("Version:0.9.2.4]\r\n\r\nready"));

  softser.begin(9600); // Soft serial connection to ESP8266
  Serial.begin(57600); while(!Serial); // UART serial debug

  Serial.println(F("Adafruit ESP8266 Demo"));

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
      if(wifi.connectTCP(F(HOST), PORT)) {

        Serial.print("Connected..");
        wifi.command(F("AT+CIPMUX=0\r\n"));
        wifi.find();
        
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
    if(do_next(count))
        count++;
    wifi.find();
}

boolean do_next(int count)
{

    switch(count){ 
        case 0:

            return wifi.sendCommand("HELO computer.com",F("250"));
                
            break;
        case 1:
            return wifi.sendCommand("AUTH LOGIN",F("334 VXNlcm5hbWU6"));
            break;
        case 2:
            return wifi.sendCommand("base64uname",F("334 UGFzc3dvcmQ6")); 
            break;
        case 3:
            return wifi.sendCommand("base64pass",F("235"));
            break;
        case 4:
            return wifi.sendCommand("MAIL FROM:<myemail@domain.com>",F("250"));
            break;
        case 5:
            return wifi.sendCommand("RCPT TO:<toemail@domain2.com>",F("250"));  
            break;
        case 6:
            return wifi.sendCommand("DATA",F("354"));
            break;
        case 7:
            return wifi.sendCommand("FROM: esp8266 <myemail@domain.com>");  
            break;
        case 8:
            return wifi.sendCommand("TO: Miguel <toemail@domain2.com>");  
            break;
        case 9:
            return wifi.sendCommand("SUBJECT: The Subject");
            break;
        case 10:
            return wifi.sendCommand("\r\n");   // marks end of header
            break;
        case 11:
            return wifi.sendCommand("Hi,My message is this");
            break;
        case 12:
            return wifi.sendCommand("\r\n.");  // marks end of data
            break;
        case 13:
            return wifi.sendCommand("QUIT");
            break;
    case 14:
        wifi.closeAP();
            wifi.closeTCP();
            return true;
            break;
    default:
        Serial.println("Done");
        return true;
            break;
        }
}
