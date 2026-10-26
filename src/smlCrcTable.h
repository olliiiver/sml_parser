#ifndef SML_CRC_TABLE_H
#define SML_CRC_TABLE_H

#include <stdint.h>

/* CRC-16/X-25 (reflected polynomial 0x8408) lookup table for one nibble.
 * Each byte is processed as two nibbles, which needs only 16 entries instead
 * of 256. */
#ifdef ARDUINO
#include <Arduino.h>
static const uint16_t smlCrcTable[16] PROGMEM =
#else
static const uint16_t smlCrcTable[16] =
#endif
    {0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
     0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F};

#endif
