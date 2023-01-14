#include "sml.h"
#include <AltSoftSerial.h>
#include <Arduino.h>
#include <RingBuf.h>
#include <stdio.h>

// Reads data from Arduino Pin 8 (only this pin is possible, see AltSoftSerial)
// SoftwareSerial will be too slow to read data even on 9600 baud.
// Dump successfully received message as hex values which
// can be inserted into a .h file.

#define MAX_BUF_SIZE 700

AltSoftSerial inputSerial;

sml_states_t currentState;

RingBuf<unsigned char, MAX_BUF_SIZE> myBuffer;

void print_buffer()
{
  unsigned int i = 0;
  unsigned int j = 0;
  char b[5];
  Serial.print(F("Size: "));
  Serial.print(myBuffer.size());
  Serial.println("");
  Serial.println(F("--- "));
  for (j = 0; j < myBuffer.size(); j++) {
    i++;
    sprintf(b, "0x%02X", myBuffer[j]);
    Serial.print(b);
    if (j < myBuffer.size() - 1) {
      Serial.print(", ");
    }
    else {
      Serial.println("");
      Serial.println(F("--- "));
    }
    if ((i % 15) == 0) {
      Serial.println("");
      i = 0;
    }
  }
}

void readByte(unsigned char currentChar)
{
  currentState = smlState(currentChar);
  if (currentState == SML_START) {
    myBuffer.clear();
    myBuffer.push(0x1B);
    myBuffer.push(0x1B);
    myBuffer.push(0x1B);
  }
  else {
    if (myBuffer.size() < MAX_BUF_SIZE) {
      myBuffer.push(currentChar);
    }
    else {
      Serial.print(F(">>> Message larger than MAX_BUF_SIZE\n"));
    }
  }
  if (currentState == SML_UNEXPECTED) {
    Serial.print(F(">>> Unexpected byte\n"));
  }
  if (currentState == SML_FINAL) {
    Serial.print(F(">>> Successfully received a complete message!\n"));
    print_buffer();
  }
  if (currentState == SML_CHECKSUM_ERROR) {
    Serial.print(F(">>> Checksum error.\n"));
  }
}

void setup()
{
  inputSerial.begin(9600);
  Serial.begin(115200);
  Serial.println(F("Starting"));
}

void loop()
{
  while (inputSerial.available() > 0) {
    readByte(inputSerial.read());
  }
}
