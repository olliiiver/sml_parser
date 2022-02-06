#include <stdio.h>
#include "sml.h"
#include "ehz_bin.h"

#define MAX_STR_MANUF 5
unsigned char manuf[MAX_STR_MANUF];
double T1Wh = -2, SumWh = -2;

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)(); 
} SMLHandler;

void Manufacturer() {
  smlOBISManufacturer(manuf, MAX_STR_MANUF);
}

void PowerT1() {
  smlOBISWh(T1Wh);
}

void PowerSum() {
  smlOBISWh(SumWh);
}

SMLHandler SMLHandlers[] = {
  {{ 0x81, 0x81, 0xc7, 0x82, 0x03, 0xff }, &Manufacturer}, /* 129-129:199.130.3*255 */
  {{ 0x01, 0x00, 0x01, 0x08, 0x01, 0xff }, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};

int main () {
  int i = 0, iHandler = 0; 
  unsigned char c;
  sml_states_t s;
  for (i = 0; i < ehz_bin_len; ++i) {
    c = ehz_bin[i];
    s = smlState(c);
    if (s == SML_START) {
      /* reset local vars */
      manuf[0] = 0; T1Wh = -3; SumWh = -3;
    }
    if (s == SML_LISTEND) {
      /* check handlers on last received list */
      for (iHandler=0; SMLHandlers[iHandler].Handler != 0 && 
            !(smlOBISCheck(SMLHandlers[iHandler].OBIS)); iHandler++);
      if (SMLHandlers[iHandler].Handler != 0) {
        SMLHandlers[iHandler].Handler(); 
      }
    }
    if (s == SML_UNEXPECTED) {
      printf(">>> Unexpected byte! <<<\n");
    }
    if (s == SML_FINAL) {
      printf(">>> FINAL! Checksum OK\n");
      printf(">>> Manufacturer.............: %s\n", manuf);
      printf(">>> Power T1    (1-0:1.8.1)..: %.3f kWh\n", T1Wh);
      printf(">>> Power T1+T2 (1-0:1.8.0)..: %.3f kWh\n\n", SumWh);
    }
  }
}