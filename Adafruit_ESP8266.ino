// This is a super simple demo program for ESP8266's that can use software serial 
// @ 9600 baud. Requires firmware that runs at 9600 baud, only tested with Adafruit
// programmed modules!

#include <SoftwareSerial.h>
#define SSID "ssid name"      //your wifi ssid here
#define PASS "password"   //your wifi wep key here

//  www.adafruit.com/testwifi/index.html
#define DST_IP "207.58.139.247" // IP for adafruit.com, below
#define HOST "www.adafruit.com"
#define WEBPAGE "/testwifi/index.html"
#define PORT  "80"

#define ESP_RST 4

// Use software serial (check to make sure these are valid softserial pins!)
#define ESP_RX 2
#define ESP_TX 3
SoftwareSerial softser(ESP_RX, ESP_TX); // RX, TX
Stream *esp = &softser;

// can also do 
// Stream *esp = &Serial1;

#define REPLYBUFFSIZ 255
char replybuffer[REPLYBUFFSIZ];
uint8_t getReply(char *send, uint16_t timeout, boolean echo = true);
uint8_t espreadline(uint16_t timeout = 500, boolean multiline = false);
boolean sendCheckReply(char *send, char *reply, uint16_t timeout = 500);

 
enum {WIFI_ERROR_NONE=0, WIFI_ERROR_AT, WIFI_ERROR_RST, WIFI_ERROR_SSIDPWD, WIFI_ERROR_SERVER, WIFI_ERROR_UNKNOWN};

void setup()
{
  pinMode(9, OUTPUT);
  pinMode(13, OUTPUT);
 
  //blink led13 to indicate power up
  for(int i = 0; i<3; i++)
  {
   digitalWrite(13,HIGH);
   delay(50);
   digitalWrite(13,LOW);
   delay(100);
  }
 
  // Serial debug console
  Serial.begin(115200);
  // Set time to wait for response strings to be found
 
  //Open software serial for chatting to ESP
  softser.begin(9600);   // requires new firmware!
  // OR use hardware serial
  //Serial1.begin(9600);
  
  Serial.println("ESP8266 Demo");
 
 
 // hard reset if you can
  pinMode(ESP_RST, INPUT);
  digitalWrite(ESP_RST, LOW);
  pinMode(ESP_RST, OUTPUT);
  delay(100);
  pinMode(ESP_RST, INPUT);
  delay(2000);
  
  //test if the module is ready
  if(! espReset()) {
    Serial.println("Module didn't respond.");
    debugLoop();
  }

  Serial.println("ESP Module is ready");

  //connect to the wifi
  byte err = setupWiFi();
  
  if (err) {
    // error, print error code
    Serial.print("setup error:");  Serial.println((int)err);

    debugLoop();
  }
  
  // success, print IP
  Serial.print("ESP setup success, my IP addr:");
  uint32_t ip = getIP();
  if (ip) {
    Serial.println(ip, HEX);
  } else {
    Serial.println("none");
  }
  
  sendCheckReply("AT+CIPSTO=0", "OK");
   
  //set the single connection mode
}

boolean ESP_GETpage(char *host, char *ip, uint16_t port, char *page) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += ip;
  cmd += "\",";
  cmd += port;
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);
  
  if (! sendCheckReply(replybuffer, "OK")) {
    sendCheckReply("AT+CIPCLOSE", "OK");
    return false;
  }
  
  String request = "GET ";
  request += page;
  request += " HTTP/1.1\r\nHost: ";
  request += host;
  request += "\r\n\r\n";
  
  cmd = "AT+CIPSEND=";
  cmd += request.length();
  cmd.toCharArray(replybuffer, REPLYBUFFSIZ);
  sendCheckReply(replybuffer, ">");

  Serial.print("Sending: "); Serial.println(request.length());
  Serial.println(F("*********SENDING*********"));
  Serial.print(request);
  Serial.println(F("*************************"));
  
  request.toCharArray(replybuffer, REPLYBUFFSIZ);

  esp->println(request);

  espreadline(100);  // this is the 'echo' from the data
  espreadline(100);  //Serial.print("1>"); Serial.println(replybuffer); // probably the 'busy s...'
  espreadline(3000); // Serial.print("2>"); Serial.println(replybuffer); 
  if (! strstr(replybuffer, "SEND OK") ) return false;
  
  espreadline(1000);  Serial.print("3>"); Serial.println(replybuffer);
  char *s = strstr(replybuffer, "+IPD,");
  if (!s) return false;
  uint16_t len = atoi(s+5);
  //Serial.print(len); Serial.println(" bytes total");

  int16_t contentlen = 0;
  while (1) {
    espreadline(50);
    s = strstr(replybuffer, "Content-Length: ");
    if (s) {
      //Serial.println(replybuffer);
      contentlen = atoi(s+16);
      Serial.print("clen = "); Serial.println(contentlen);
    }
    s = strstr(replybuffer, "Content-Type: ");
    if (s && contentlen) {
      int16_t i;
      char c;
      
      for (i=-2; i<contentlen; i++) {  // eat the first 2 chars (\n\r)
        while (!esp->available());
        c = esp->read(); //UDR0 = c;
        if (i >= 0) {
          replybuffer[i] = c;
        }
      }
      replybuffer[i] = 0;
      return true;
    }
  }
  //while (1) {
  //  if (esp.available()) UDR0 = esp.read();
  //} 
}
 
void loop()
{
  ESP_GETpage(HOST, DST_IP, 80, WEBPAGE);
  
  Serial.println(F("**********REPLY***********"));
  Serial.print(replybuffer);
  Serial.println(F("**************************"));

  sendCheckReply("AT+CIPCLOSE", "OK");
  
  debugLoop();
  
  delay(1000);
  
  while (1);
  
}

boolean espReset() {
  getReply("AT+RST", 1000, true);
  if (! strstr(replybuffer, "OK")) return false;
  delay(2000);
  return true;
}

boolean ESPconnectAP(char *s, char *p) {
  
  getReply("AT+CWMODE=1", 500, true);
  if (! (strstr(replybuffer, "OK") || strstr(replybuffer, "no change")) ) 
    return false;
    
  String connectStr = "AT+CWJAP=\"";
  connectStr += SSID;
  connectStr += "\",\"";
  connectStr += PASS;
  connectStr += "\"";
  connectStr.toCharArray(replybuffer, REPLYBUFFSIZ);
  getReply(replybuffer, 500, true);
  espreadline(5000);
  Serial.print("<-- "); Serial.println(replybuffer);  

  return (strstr(replybuffer, "OK") != 0);
}
 
 
byte setupWiFi() {
  // reset WiFi module
  Serial.println("Resetting...");
  if (!espReset()) 
    return WIFI_ERROR_RST;

  delay(1000);
  
  Serial.println("Checking for ESP AT");

  if (!sendCheckReply("AT", "OK"))
    return WIFI_ERROR_AT;
  
  Serial.print("Connecting to "); Serial.println(SSID);
  if (!ESPconnectAP(SSID, PASS))
    return WIFI_ERROR_SSIDPWD;
 
  Serial.println("Single Client");
  if (!sendCheckReply("AT+CIPMUX=0", "OK"))
        return WIFI_ERROR_SERVER;
 
  return WIFI_ERROR_NONE;
}  

uint32_t getIP() {
  getReply("AT+CIFSR", 500, true);

  return 0;
}




/************************/
uint8_t espreadline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;
  
  while (timeout--) {
    if (replyidx > REPLYBUFFSIZ-1) break;
    
    while(esp->available()) {
      char c =  esp->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;
        
        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      // Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }
    
    if (timeout == 0) break;
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

uint8_t getReply(char *send, uint16_t timeout, boolean echo) {
  // flush input
  while(esp->available()) {
     esp->read();
  }
  
  if (echo) {
    Serial.print("---> "); Serial.println(send); 
  }
  esp->println(send);
  
  // eat first reply sentence (echo)
  espreadline(timeout);  
  
  uint8_t l = espreadline();  
  if (echo) {
    Serial.print ("<--- "); Serial.println(replybuffer);
  }
  return l;
}

boolean sendCheckReply(char *send, char *reply, uint16_t timeout) {
  getReply(send, timeout, true);

/*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
    Serial.print(replybuffer[i], HEX); Serial.print(" ");
  }
  Serial.println();
  for (uint8_t i=0; i<strlen(reply); i++) {
    Serial.print(reply[i], HEX); Serial.print(" ");
  }
  Serial.println();
  */
  return (strcmp(replybuffer, reply) == 0);
}

void debugLoop() {
  Serial.println("========================");
  //serial loop mode for diag
  while(1) {
    while (Serial.available()) {
      esp->write(Serial.read());
    }
    while (esp->available()) {
      Serial.write(esp->read());
    }
  }
}
