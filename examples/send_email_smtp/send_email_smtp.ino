/* 
--------------------------------------------------------------------------------------------------------------------------
-- @description A sketch for sending email via SMTP. Working Example: https://www.youtube.com/watch?v=n5WZ_BNRvRY 
-- You must use an account which can provide unencrypted authenticated access.
-- This example was tested with an AOL and Time Warner email accounts. GMail does not offer unecrypted authenticated access.
-- To obtain your email's SMTP server and port simply Google it e.g. [my email domain] SMTP settings
-- For example for timewarner you'll get to this page http://www.timewarnercable.com/en/support/faqs/faqs-internet/e-mailacco/incoming-outgoing-server-addresses.html
-- To Learn more about SMTP email visit:
-- SMTP Commands Reference - http://www.samlogic.net/articles/smtp-commands-reference.htm
-- See "SMTP transport example" in this page http://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol
-----------------------------------------------------------------------------------------------------------------------------
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

#define ESP_RX   3
#define ESP_TX   4
#define ESP_RST  8
SoftwareSerial softser(ESP_RX, ESP_TX);

// Must declare output stream before Adafruit_ESP8266 constructor; can be
// a SoftwareSerial stream, or Serial/Serial1/etc. for UART.
Adafruit_ESP8266 wifi(&softser, &Serial, ESP_RST);
// Must call begin() on the stream(s) before using Adafruit_ESP8266 object.

#define ESP_SSID "networkName" // Your network name here
#define ESP_PASS "networkPassword" // Your network password here

char EMAIL_FROM[] = "yourEmail@domain.com";
char EMAIL_PASSWORD[] =  "yourEmail'sPassword";
char EMAIL_TO[] = "toEmail@domain2.com";
char SUBJECT[]  = "My ESP8266";
char EMAIL_CONTENT[] = "Hello,\r\nThis is a message from your ESP8266.";

// We'll need your EMAIL_FROM and its EMAIL_PASSWORD base64 encoded, you can use https://www.base64encode.org/
#define EMAIL_FROM_BASE64 "yourEmailBase64Encoded"
#define EMAIL_PASSWORD_BASE64 "yourEmail'sPasswordBase64Encoded"

#define HOST     "mail.domain.com"     // Find/Google your email provider's SMTP outgoing server name for unencrypted email

#define PORT     587                     // Find/Google your email provider's SMTP outgoing port for unencrypted email

int count = 0; // we'll use this int to keep track of which command we need to send next
bool send_flag = false; // we'll use this flag to know when to send the email commands

void setup() {
  char buffer[50];

  // This might work with other firmware versions (no guarantees)
  // by providing a string to ID the tail end of the boot message:
  
  // comment/replace this if you are using something other than v 0.9.2.4!
  wifi.setBootMarker(F("Version:0.9.2.4]\r\n\r\nready"));

  softser.begin(9600); // Soft serial connection to ESP8266
  Serial.begin(57600); while(!Serial); // UART serial debug

  Serial.println(F("Adafruit ESP8266 Email"));

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
        Serial.println("Type \"send it\" to send an email");
        
    } else { // IP addr check failed
      Serial.println(F("error"));
    }
  } else { // WiFi connection failed
    Serial.println(F("FAIL"));
  }
}

void loop() {

    if(!send_flag){ // check if we expect to send an email
        if(Serial.available()){  // there is data in the serial, let's see if the users wants to "send it" [the email]
            if(Serial.find("send it")){  // set the send_flag when the uses types "send it" in the serial monitor.
                Serial.println("Sending email...");
                send_flag = true;
            }
        }
    }

    if(send_flag){ // the send_flat is set, this means we are or need to start sending SMTP commands
        if(do_next()){ // execute the next command
            count++; // increment the count so that the next command will be executed next time.
        }
    }
}

// do_next executes the SMTP command in the order required.
boolean do_next()
{

    switch(count){ 
    case 0:
        Serial.println("Connecting...");
        return wifi.connectTCP(F(HOST), PORT);
        break;
    case 1:
        // send "HELO ip_address" command. Server will reply with "250" and welcome message
        return wifi.cipSend("HELO computer.com",F("250")); // ideally an ipaddress should go in place 
                                                           // of "computer.com" but I think the email providers
                                                           // check the IP anyways so I just put anything.                                   
        break;
    case 2:
        // send "AUTH LOGIN" command to the server will reply with "334 username" base 64 encoded
        return wifi.cipSend("AUTH LOGIN",F("334 VXNlcm5hbWU6"));
        break;
    case 3:
        // send username/email base 64 encoded, the server will reply with "334 password" base 64 encoded
        return wifi.cipSend(EMAIL_FROM_BASE64,F("334 UGFzc3dvcmQ6")); 
        break;
    case 4:
        // send password base 64 encoded, upon successful login the server will reply with 235.
        return wifi.cipSend(EMAIL_PASSWORD_BASE64,F("235"));
        break;
    case 5:{
        // send "MAIL FROM:<emali_from@domain.com>" command
        char mailFrom[50] = "MAIL FROM:<"; // If 50 is not long enough change it, do the same for the array in the other cases
        strcat(mailFrom,EMAIL_FROM);
        strcat(mailFrom,">");

        return wifi.cipSend(mailFrom,F("250"));
        break;
    }
    case 6:{
        // send "RCPT TO:<email_to@domain.com>" command
        char rcptTo[50] = "RCPT TO:<";
        strcat(rcptTo,EMAIL_TO);
        strcat(rcptTo,">");
        return wifi.cipSend(rcptTo,F("250"));  
        break;
    }
    case 7:
        // Send "DATA"  command, the server will reply with something like "334 end message with \r\n.\r\n."
        return wifi.cipSend("DATA",F("354"));
        break;
    case 8:{
        // apply "FROM: from_name <from_email@domain.com>" header
        char from[100] = "FROM: ";
        strcat(from,EMAIL_FROM);
        strcat(from," ");
        strcat(from,"<");
        strcat(from,EMAIL_FROM);
        strcat(from,">");
        return wifi.cipSend(from);  
        break;
    }
    case 9:{
        // apply TO header 
        char to[100] = "TO: ";
        strcat(to,EMAIL_TO);
        strcat(to,"<");
        strcat(to,EMAIL_TO);
        strcat(to,">");
        return wifi.cipSend(to);  
        break;
    }
    case 10:{
        // apply SUBJECT header
        char subject[50] = "SUBJECT: ";
        strcat(subject,SUBJECT);
        return wifi.cipSend(subject);
        break;
    }
    case 11:
        return wifi.cipSend("\r\n");   // marks end of header (SUBJECT, FROM, TO, etc);
        break;
    case 12:
        return wifi.cipSend(EMAIL_CONTENT);
        break;
    case 13:
        return wifi.cipSend("\r\n.");  // marks end of data command
        break;
    case 14:
        return wifi.cipSend("QUIT");
        break;
    case 15:
        wifi.closeTCP();
        return true;
        break;
    case 16:
        Serial.println("Done");
        send_flag = false;
        count = 0;
        return false; // we don't want to increment the count
        break;
    default:
        break;
        }
}
