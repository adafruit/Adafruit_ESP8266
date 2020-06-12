/*!
 * @file Adafruit_ESP8266.cpp
 *
 * @mainpage Adafruit ESP8266 library
 *
 * @section intro_sec Introduction
 *
 * An Arduino library for the ESP8266 WiFi-serial bridge
 *
 * https://www.adafruit.com/product/2282
 *
 * The ESP8266 is a 3.3V device.  Safe operation with 5V devices (most
 * Arduino boards) requires a logic-level shifter for TX and RX signals.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried and Phil Burgess for Adafruit Industries.
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include "Adafruit_ESP8266.h"

/**************************************************************************/
/*!
    @brief  Constructor
    @param    serial_stream
              Pointer to Stream (Hardware or Software Serial) for the ESP8266
    @param    debug_stream
              Pointer to Stream (Hardware or Software Serial) for debug out
    @param resetpin Reset pin
*/
/**************************************************************************/
Adafruit_ESP8266::Adafruit_ESP8266(Stream *serial_stream, Stream *debug_stream,
                                   int8_t resetpin)
    : stream(serial_stream), debug(debug_stream), reset_pin(resetpin),
      host(NULL), writing(false) {
  setTimeouts();
};

//
/**************************************************************************/
/*!
    @brief   Override various timings.Passing 0 for an item keeps current
   setting.
    @param  rcv  Receive data timeout
    @param  rst  Reset response timeout
    @param  con  Connection timeout
    @param  ipd  IPD response timeout
*/
/**************************************************************************/
void Adafruit_ESP8266::setTimeouts(uint32_t rcv, uint32_t rst, uint32_t con,
                                   uint32_t ipd) {
  if (rcv) {
    stream->setTimeout(rcv);
    receiveTimeout = rcv;
  }
  if (rst)
    resetTimeout = rst;
  if (con)
    connectTimeout = con;
  if (ipd)
    ipdTimeout = ipd;
}

/**************************************************************************/
/*!
    @brief  Override boot marker string, or pass NULL to restore default.
    @param  str
            The bootmarker string to look for
*/
/**************************************************************************/
void Adafruit_ESP8266::setBootMarker(Fstr *str) {
  bootMarker = str ? str : defaultBootMarker;
}

/**************************************************************************/
/*!
    @brief  Send one byte character. Anything printed to the EPS8266 object will
   be split to both the WiFi and debug streams.  Saves having to print
   everything twice in debug code.
    @param  c
            The character
*/
/**************************************************************************/
size_t Adafruit_ESP8266::write(uint8_t c) {
  if (debug) {
    if (!writing) {
      debug->print(F("---> "));
      writing = true;
    }
    debug->write(c);
  }
  return stream->write(c);
}

/**************************************************************************/
/*!
    @brief
    Equivalent to Arduino Stream find() function, but with search string in
   flash/PROGMEM rather than RAM-resident. Can optionally pass NULL (or no
   argument) to read/purge the OK+CR/LF returned by most AT commands.  The ipd
   flag indicates this call follows a CIPSEND request and might be broken into
   multiple sections with +IPD delimiters, which must be parsed and handled (as
   the search string may cross these delimiters and/or contain \\r or \\n
   itself).
    @param  str The Flash string to find
    @param ipd If we're looking for IPD data
    @returns  Returns true if string found
    (any further pending input remains in stream), false if timeout occurs.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::find(Fstr *str, boolean ipd) {
  uint8_t stringLength, matchedLength = 0;
  int c;
  boolean found = false;
  uint32_t t, save;
  uint16_t bytesToGo = 0;

  if (ipd) { // IPD stream stalls really long occasionally, what gives?
    save = receiveTimeout;
    setTimeouts(ipdTimeout);
  }

  if (str == NULL)
    str = F("OK\r\n");
  stringLength = strlen_P((Pchr *)str);

  if (debug && writing) {
    debug->print(F("<--- '"));
    writing = false;
  }

  for (t = millis();;) {
    if (ipd && !bytesToGo) {  // Expecting next IPD marker?
      if (find(F("+IPD,"))) { // Find marker in stream
        for (;;) {
          if ((c = stream->read()) > 0) { // Read subsequent chars...
            if (debug)
              debug->write(c);
            if (c == ':')
              break;                                  // ...until delimiter.
            bytesToGo = (bytesToGo * 10) + (c - '0'); // Keep count
            t = millis();     // Timeout resets w/each byte received
          } else if (c < 0) { // No data on stream, check for timeout
            if ((millis() - t) > receiveTimeout)
              goto bail;
          } else
            goto bail; // EOD on stream
        }
      } else
        break; // OK (EOD) or ERROR
    }

    if ((c = stream->read()) > 0) { // Character received?
      if (debug)
        debug->write(c); // Copy to debug stream
      bytesToGo--;
      if (c == pgm_read_byte((Pchr *)str + matchedLength)) { // Match next byte?
        if (++matchedLength == stringLength) { // Matched whole string?
          found = true;                        // Winner!
          break;
        }
      } else { // Character mismatch; reset match pointer+counter
        matchedLength = 0;
      }
      t = millis();     // Timeout resets w/each byte received
    } else if (c < 0) { // No data on stream, check for timeout
      if ((millis() - t) > receiveTimeout)
        break; // You lose, good day sir
    } else
      break; // End-of-data on stream
  }

bail: // Sorry, dreaded goto.  Because nested loops.
  if (debug)
    debug->println('\'');
  if (ipd)
    setTimeouts(save);
  return found;
}

/**************************************************************************/
/*!
    @brief Read from ESP8266 stream into RAM, up to a given size.  Max number of
   chars read is 1 less than this, so NUL can be appended on string.
    @param buf Where to put streamed in data
    @param bufSiz The max allocated buffer size
    @returns Bytes read
*/
/**************************************************************************/
int Adafruit_ESP8266::readLine(char *buf, int bufSiz) {
  if (debug && writing) {
    debug->print(F("<--- '"));
    writing = false;
  }
  int bytesRead = stream->readBytesUntil('\r', buf, bufSiz - 1);
  buf[bytesRead] = 0;
  if (debug) {
    debug->print(buf);
    debug->println('\'');
  }
  while (stream->read() != '\n')
    ; // Discard thru newline
  return bytesRead;
}

/**************************************************************************/
/*!
    @brief ESP8266 is reset by momentarily connecting RST to GND.  Level
   shifting is not necessary provided you don't accidentally set the pin to HIGH
   output. It's generally safe-ish as the default Arduino pin state is INPUT
   (w/no pullup) -- setting to LOW provides an open-drain for reset.
    @returns true if expected boot message is received (or if RST is unused)
   false otherwise.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::hardReset(void) {
  if (reset_pin < 0)
    return true;
  digitalWrite(reset_pin, LOW);
  pinMode(reset_pin, OUTPUT); // Open drain; reset -> GND
  delay(10);                  // Hold a moment
  pinMode(reset_pin, INPUT);  // Back to high-impedance pin state
  return find(bootMarker);    // Purge boot message from stream
}

/**************************************************************************/
/*!
    @brief Software reset
    @returns true if expected boot message is received (or if RST is unused)
   false otherwise.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::softReset(void) {
  boolean found = false;
  uint32_t save = receiveTimeout; // Temporarily override recveive timeout,
  receiveTimeout = resetTimeout;  // reset time is longer than normal I/O.
  println(F("AT+RST"));           // Issue soft-reset command
  if (find(bootMarker)) {         // Wait for boot message
    println(F("ATE0"));           // Turn off echo
    found = find();               // OK?
  }
  receiveTimeout = save; // Restore normal receive timeout
  return found;
}

/**************************************************************************/
/*!
    @brief For interactive debugging...shuttle data between Serial Console <->
   WiFi
*/
/**************************************************************************/
void Adafruit_ESP8266::debugLoop(void) {
  if (!debug)
    for (;;)
      ; // If no debug connection, nothing to do.

  debug->println(F("\n========================"));
  for (;;) {
    if (debug->available())
      stream->write(debug->read());
    if (stream->available())
      debug->write(stream->read());
  }
}

/**************************************************************************/
/*!
    @brief Connect to WiFi access point.  SSID and password are flash-resident
   strings.  May take several seconds to execute, this is normal.
    @param ssid Flash string of AP SSID
    @param pass Flash string of AP password
    @returns true on successful connection, false otherwise.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::connectToAP(Fstr *ssid, Fstr *pass) {
  char buf[256];

  println(F("AT+CWMODE=1")); // WiFi mode = Sta
  readLine(buf, sizeof(buf));
  if (!(strstr_P(buf, (Pchr *)F("OK")) ||
        strstr_P(buf, (Pchr *)F("no change"))))
    return false;

  print(F("AT+CWJAP=\"")); // Join access point
  print(ssid);
  print(F("\",\""));
  print(pass);
  println('\"');
  uint32_t save = receiveTimeout;  // Temporarily override recv timeout,
  receiveTimeout = connectTimeout; // connection time is much longer!
  boolean found = find();          // Await 'OK' message
  receiveTimeout = save;           // Restore normal receive timeout
  if (found) {
    println(F("AT+CIPMUX=0")); // Set single-client mode
    found = find();            // Await 'OK'
  }

  return found;
}

/**************************************************************************/
/*!
    @brief  Close current connection
*/
/**************************************************************************/
void Adafruit_ESP8266::closeAP(void) {
  println(F("AT+CWQAP")); // Quit access point
  find();                 // Purge 'OK'
}

/**************************************************************************/
/*!
    @brief  Open TCP connection.  Hostname is flash-resident string.
    @param hoststr Hostname string
    @param port TCP port on host
    @returns true on successful connection, else false.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::connectTCP(Fstr *hoststr, int port) {

  print(F("AT+CIPSTART=\"TCP\",\""));
  print(hoststr);
  print(F("\","));
  println(port);

  if (find(F("Linked"))) {
    host = hoststr;
    return true;
  }
  return false;
}
/**************************************************************************/
/*!
    @brief  Close current connection
*/
/**************************************************************************/
void Adafruit_ESP8266::closeTCP(void) {
  println(F("AT+CIPCLOSE"));
  find(F("Unlink\r\n"));
}

/**************************************************************************/
/*!
    @brief  Requests page from currently-open TCP connection. URL is
   flash-resident string.
    @param url The flash string URL to request
    @returns true if request issued successfully, else false.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::requestURL(Fstr *url) {
  print(F("AT+CIPSEND="));
  println(25 + strlen_P((Pchr *)url) + strlen_P((Pchr *)host));
  if (find(F("> "))) { // Wait for prompt
    print(F("GET "));  // 4
    print(url);
    print(F(" HTTP/1.1\r\nHost: ")); // 17
    print(host);
    print(F("\r\n\r\n")); // 4
    return (find());      // Gets 'SEND OK' line
  }
  return false;
}

/**************************************************************************/
/*!
    @brief Requests page from cur
rently-open TCP connection.  URL is character string in SRAM. Calling function
should then handle data returned, may need to parse IPD delimiters (see notes in
find() function. (Can call find(F("Unlink"), true) to dump to debug.)
    @param url The SRAM string URL to request
   @returns true if request issued successfully, else false.
*/
/**************************************************************************/
boolean Adafruit_ESP8266::requestURL(char *url) {
  print(F("AT+CIPSEND="));
  println(25 + strlen(url) + strlen_P((Pchr *)host));
  if (find(F("> "))) { // Wait for prompt
    print(F("GET "));  // 4
    print(url);
    print(F(" HTTP/1.1\r\nHost: ")); // 17
    print(host);
    print(F("\r\n\r\n")); // 4
    return (find());      // Gets 'SEND OK' line
  }
  return false;
}
