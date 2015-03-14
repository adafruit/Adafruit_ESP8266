#include <Adafruit_ESP8266_SMTP.h>

Adafruit_ESP8266_SMTP::Adafruit_ESP8266_SMTP(Stream *s, Stream *d, int8_t r):Adafruit_ESP8266(s,d,r){

};

boolean Adafruit_ESP8266_SMTP::sendCommand(const char* command,Fstr *ack){
    print(F("AT+CIPSEND="));
    println(2 + strlen(command));
    if(find(F("> "))) { // Wait for prompt
        print(command);
        print(F("\r\n")); // 4
        if(ack!=NULL){
            return(find(ack));
        }else{
            return(find()); // Gets 'SEND OK' line
        }
    }
    return false;
}
