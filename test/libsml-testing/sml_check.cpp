#include "sml.h"
#include <stdio.h>

/*
 * Parse a binary SML dump and check that
 *  - at least one message was received with a valid checksum
 *  - no checksum errors occurred
 *  - 1-0:1.8.0*255 provides a value
 *
 * Known deviations of a dump can be relaxed with the options:
 *  --allow-crc-errors  dump contains messages with invalid checksums
 *  --no-1.8.0          meter does not send 1-0:1.8.0*255
 *  --broken            dump contains no complete message, only check that
 *                      parsing does not crash
 */
#include <string.h>

static const unsigned char OBIS_1_8_0[6] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};

int main(int argc, char *argv[])
{
  bool allowCrcErrors = false, need180 = true, broken = false;
  const char *file = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--allow-crc-errors") == 0) {
      allowCrcErrors = true;
    }
    else if (strcmp(argv[i], "--no-1.8.0") == 0) {
      need180 = false;
    }
    else if (strcmp(argv[i], "--broken") == 0) {
      broken = true;
    }
    else {
      file = argv[i];
    }
  }
  if (!file) {
    fprintf(stderr,
            "usage: %s [--allow-crc-errors] [--no-1.8.0] [--broken] "
            "<file.bin>\n",
            argv[0]);
    return 2;
  }

  FILE *f = fopen(file, "rb");
  if (!f) {
    perror(file);
    return 2;
  }

  int finals = 0, crcErrors = 0, values = 0, ch;
  double wh = -1;
  sml_states_t s;

  while ((ch = fgetc(f)) != EOF) {
    unsigned char c = (unsigned char)ch;
    s = smlState(c);
    if (s == SML_LISTEND && smlOBISCheck(OBIS_1_8_0)) {
      smlOBISWh(wh);
      if (wh >= 0) {
        values++;
      }
    }
    if (s == SML_FINAL) {
      finals++;
    }
    if (s == SML_CHECKSUM_ERROR) {
      crcErrors++;
    }
  }
  fclose(f);

  printf("messages: %d, crc errors: %d, 1.8.0 values: %d (last %.1f Wh)\n",
         finals, crcErrors, values, wh);

  if (broken) {
    return 0;
  }
  if (finals == 0 || (crcErrors > 0 && !allowCrcErrors) ||
      (values == 0 && need180)) {
    return 1;
  }
  return 0;
}
