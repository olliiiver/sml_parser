#include "ehz_bin.h"
#include "sml.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * Read values without knowing the OBIS codes of the meter at compile time:
 *  - print every entry the meter sends ("info mode", see issue #14)
 *  - watch OBIS codes given as strings, e.g. from a configuration (#28)
 */

#define MAX_WATCHED 4
#define MAX_STR_OBIS 24

/* e.g. read from a config file, web interface or EEPROM */
const char *config[] = {"1-0:1.8.0*255", "1-0:15.7.0", 0};

typedef struct {
  unsigned char obis[6];
  double value;
} Watched;

Watched watched[MAX_WATCHED];
int watchedCount = 0;

void printEntry(const sml_obis_entry_t &entry)
{
  char obis[MAX_STR_OBIS];
  int i;
  smlOBISToString(entry.obis, obis, MAX_STR_OBIS);
  printf("%-22s", obis);
  if (entry.type == SML_DATA_SIGNED_INT ||
      entry.type == SML_DATA_UNSIGNED_INT) {
    printf("%.3f (unit %d)\n", entry.value * pow(10, entry.scaler), entry.unit);
  }
  else {
    for (i = 0; i < entry.length; i++) {
      printf("%02X ", entry.bytes[i]);
    }
    printf("\n");
  }
}

int main()
{
  int i = 0, w = 0;
  unsigned char c;
  sml_states_t s;
  sml_obis_entry_t entry;
  char obis[MAX_STR_OBIS];

  for (i = 0; config[i] != 0 && watchedCount < MAX_WATCHED; i++) {
    if (smlOBISFromString(config[i], watched[watchedCount].obis)) {
      watched[watchedCount++].value = -1;
    }
    else {
      printf("Invalid OBIS code in config: %s\n", config[i]);
    }
  }

  for (i = 0; i < ehz_bin_len; ++i) {
    c = ehz_bin[i];
    s = smlState(c);
    if (s == SML_LISTEND && smlOBISEntry(entry)) {
      printEntry(entry);
      for (w = 0; w < watchedCount; w++) {
        if (memcmp(watched[w].obis, entry.obis, 6) == 0) {
          watched[w].value = entry.value * pow(10, entry.scaler);
        }
      }
    }
    if (s == SML_FINAL) {
      printf(">>> FINAL! Checksum OK\n");
      for (w = 0; w < watchedCount; w++) {
        smlOBISToString(watched[w].obis, obis, MAX_STR_OBIS);
        printf(">>> %-18s: %.3f\n", obis, watched[w].value);
      }
    }
  }
}
