#ifndef _ADAFRUIT_ESP8266_SMTP_H_
#define _ADAFRUIT_ESP8266_SMTP_H_

#include <Arduino.h>
#include <Adafruit_ESP8266.h>

class  Adafruit_ESP8266_SMTP : public Adafruit_ESP8266 {
    public: 
        Adafruit_ESP8266_SMTP(Stream *s = &Serial, Stream *d = NULL, int8_t r = -1);
        boolean sendCommand(const char* command,Fstr *ack = NULL);
};

#endif _ADAFRUIT_ESP8266_SMTP_H
