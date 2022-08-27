#include "ehz_bin.h"
#include "sml.h"
#include <Arduino.h>
#include <stdio.h>

// Continuously loops through a static message from RAM and outputs information
// to serial

double T1Wh = -2, SumWh = -2;

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)();
} OBISHandler;

void PowerT1() { smlOBISWh(T1Wh); }

void PowerSum() { smlOBISWh(SumWh); }

// clang-format off
OBISHandler OBISHandlers[] = {
  {{ 0x01, 0x00, 0x01, 0x08, 0x01, 0xff }, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};
// clang-format on

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Starting"));
}

void loop()
{
  char floatBuffer[20];
  unsigned int i = 0, iHandler = 0;
  unsigned char c;
  sml_states_t s;
  while (true) {
    for (i = 0; i < ehz_bin_len; ++i) {
      c = ehz_bin[i];
      s = smlState(c);
      if (s == SML_START) {
        /* reset local vars */
        T1Wh = -3;
        SumWh = -3;
      }
      if (s == SML_LISTEND) {
        /* check handlers on last received list */
        for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                           !(smlOBISCheck(OBISHandlers[iHandler].OBIS));
             iHandler++)
          ;
        if (OBISHandlers[iHandler].Handler != 0) {
          OBISHandlers[iHandler].Handler();
        }
      }
      if (s == SML_UNEXPECTED) {
        Serial.print(F(">>> Unexpected byte! <<<\n"));
      }
      if (s == SML_FINAL) {
        Serial.print(F("Power T1    (1-0:1.8.1)..: "));
        dtostrf(T1Wh, 10, 3, floatBuffer);
        Serial.print(floatBuffer);
        Serial.print(F("\n"));

        Serial.print(F("Power T1+T2 (1-0:1.8.0)..: "));
        dtostrf(SumWh, 10, 3, floatBuffer);
        Serial.print(floatBuffer);
        Serial.print(F("\n\n\n\n"));
      }
    }
  }
}
