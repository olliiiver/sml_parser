#include "sml.h"
#include <AltSoftSerial.h>
#include <Arduino.h>
#include <stdio.h>

// Reads data from Arduino Pin 8 (only this pin is possible, see AltSoftSerial)
// SoftwareSerial will be too slow to read data even on 9600 baud.

AltSoftSerial inputSerial;

sml_states_t currentState;

void readByte(unsigned char currentChar)
{
  currentState = smlState(currentChar);
  if (currentState == SML_UNEXPECTED) {
    Serial.print(F(">>> Unexpected byte\n"));
  }
  if (currentState == SML_FINAL) {
    Serial.print(F(">>> Successfully received a complete message!\n"));
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
