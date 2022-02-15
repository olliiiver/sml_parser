#include <stdio.h>
#include <string.h>

#include "smlCrcTable.h"
#include "sml.h"

#ifdef SML_DEBUG
char logBuff[200];

#ifdef SML_NATIVE
#define SML_LOG(...) do { printf(  __VA_ARGS__); } while (0)
#define SML_TREELOG(level, ...) do { printf("%.*s", level, "        "); printf(  __VA_ARGS__); } while (0)
#elif ARDUINO
#include <Arduino.h>
#define SML_LOG(...) do { sprintf(logBuff, __VA_ARGS__); Serial.print(logBuff); } while (0)
#define SML_TREELOG(level, ...) do { sprintf(logBuff, __VA_ARGS__); Serial.print(logBuff); } while (0)
#endif

#else
#define SML_LOG(...) do {  } while (0)
#define SML_TREELOG(level, ...) do {  } while (0)
#endif

#define MAX_LIST_SIZE 64
#define MAX_TREE_SIZE 10

static sml_states_t currentState = SML_START;
static char nodes[MAX_TREE_SIZE];
static unsigned char currentLevel = 0;
static unsigned short crc = 0xFFFF;
static unsigned short crcMine = 0xFFFF;
static unsigned short crcReceived = 0x0000;
static unsigned char len = 4;
static unsigned char listBuffer[MAX_LIST_SIZE]; /* keeps a two dimensional list as length + data array */
static unsigned char listPos = 0;

void crc16(unsigned char & byte) {
#ifdef ARDUINO
  crc = pgm_read_word_near(&smlCrcTable[(byte ^ crc) & 0xff]) ^ (crc >> 8 & 0xff);
#else
  crc = smlCrcTable[(byte ^ crc) & 0xff] ^ (crc >> 8 & 0xff);
#endif
}

void setState (sml_states_t state, int byteLen) {
  currentState = state;
  len = byteLen;
}

void pushListBuffer(unsigned char byte) {
  if (listPos < MAX_LIST_SIZE) {
    listBuffer[listPos++] = byte;
  }
}

void reduceList() {
  if (currentLevel >= 0 && nodes[currentLevel] > 0) nodes[currentLevel]--; 
}

void checkMagicByte (unsigned char & byte) {
  unsigned int size = 0;
  while (currentLevel > 0 && nodes[currentLevel] == 0) {
    /* go back in tree if no nodes remaining */
    SML_TREELOG(currentLevel, "back to previous list\n");
    currentLevel--;
    listPos = 0;
  }
  if (byte > 0x70 && byte < 0x7F) {
    /* new list */
    size = byte & 0x0F;
    reduceList();
    if (currentLevel < MAX_TREE_SIZE) currentLevel++;
    nodes[currentLevel] = size;
    SML_TREELOG(currentLevel, "LISTSTART on level %i with %i nodes\n", currentLevel, size);
    setState(SML_LISTSTART, size);
    listPos = 0;
  } else if (byte >= 0x01 && byte <= 0x6F && nodes[currentLevel] > 0) {
    if (byte == 0x01) {
      /* no data, get next */
      SML_TREELOG(currentLevel, " Data %i (empty)\n", nodes[currentLevel]);
      pushListBuffer(0);
      if (nodes[currentLevel] == 1) {
        setState(SML_LISTEND, 1);
        SML_TREELOG(currentLevel, "LISTEND\n");
      } else {
        setState(SML_NEXT, 1); 
      }
    } else {
      size = (byte & 0x0F) - 1;
      SML_TREELOG(currentLevel, " Data %i (length = %i): ", nodes[currentLevel], size);
      pushListBuffer(size);
      setState(SML_DATA, size);
    }
    reduceList();
  } else if (byte == 0x00) {
    /* end of block */
    reduceList();
    SML_TREELOG(currentLevel, "End of block at level %i\n", currentLevel);
    if (currentLevel == 0) {
      setState(SML_NEXT, 1);
    } else {
      setState(SML_BLOCKEND, 1);
    }
  } else if (byte >= 0x80 && byte <= 0x8F) {
    setState(SML_HDATA, (byte & 0x0F) << 4);
  } else if (byte == 0x1B && currentLevel == 0) {
    /* end sequence */
    setState(SML_END, 3); 
  } else {
    /* Unexpected Byte */
    SML_TREELOG(currentLevel, "UNEXPECTED magicbyte >%02X< at currentLevel %i\n", byte, currentLevel);
    setState(SML_UNEXPECTED, 4);
  }
}

sml_states_t smlState (unsigned char & currentByte) {
  unsigned char size;
  if (len > 0) len--;
  crc16(currentByte);
  switch (currentState) {
    case SML_UNEXPECTED:
    case SML_CHECKSUM_ERROR:
    case SML_FINAL:
    case SML_START:
      currentState = SML_START;
      if (currentByte != 0x1b) setState(SML_UNEXPECTED, 4);
      if (len == 0) {
        SML_TREELOG(0, "START\n");
        /* completely clean any garbage from crc checksum */
        crc = 0xFFFF;
        currentByte = 0x1b;
        crc16(currentByte);
        crc16(currentByte);
        crc16(currentByte);
        crc16(currentByte);
        setState(SML_VERSION, 4);
      }
    break;
    case SML_VERSION:
      if (currentByte != 0x01) setState(SML_UNEXPECTED, 4);
      if (len == 0) {
        setState(SML_BLOCKSTART, 1);
      }
    break;
    case SML_END:
      if (currentByte != 0x1b) {
        SML_LOG("UNEXPECTED char >%02X< at SML_END\n", currentByte);
        setState(SML_UNEXPECTED, 4);
      }
      if (len == 0) {
        setState(SML_CHECKSUM, 4);
      }
    break;
    case SML_CHECKSUM:
      // SML_LOG("CHECK: %02X\n", currentByte);
      if (len == 2) {
        crcMine = crc ^ 0xFFFF;
      }
      if (len == 1) {
        crcReceived += currentByte;
      }
      if (len == 0) {
        crcReceived = crcReceived | (currentByte << 8);
        SML_LOG("Received checksum: %02X\n", crcReceived);
        SML_LOG("Calculated checksum: %02X\n", crcMine);
        if (crcMine == crcReceived) {
          setState(SML_FINAL, 4);
        } else {
          setState(SML_CHECKSUM_ERROR, 4);
        }
        crc = 0xFFFF; crcReceived = 0x000; /* reset CRC */
      }
    break;
    case SML_HDATA: 
      size = len + currentByte-1;
      setState(SML_DATA, size);
      pushListBuffer(size);
      SML_TREELOG(currentLevel, " Data (length = %i): ", size);
    break;
    case SML_DATA:
      SML_LOG("%02X ", currentByte);
      pushListBuffer(currentByte);
      if (nodes[currentLevel] == 0 && len == 0) {
        SML_LOG("\n");
        SML_TREELOG(currentLevel, "LISTEND on level %i\n", currentLevel);
        currentState = SML_LISTEND; 
      } else if (len == 0) {
        currentState = SML_DATAEND;
        SML_LOG("\n");
      }
    break;
    case SML_DATAEND:
    case SML_NEXT:
    case SML_LISTSTART:
    case SML_LISTEND:
    case SML_BLOCKSTART:
    case SML_BLOCKEND:
      checkMagicByte(currentByte);
    break;
  }
  return currentState;
}

bool smlOBISCheck(const unsigned char * obis) {
  return (memcmp(obis, &listBuffer[1], 6) == 0);
}

void smlOBISManufacturer(unsigned char * str, int maxSize) {
  int i = 0, pos = 0, size = 0;
  while (i < listPos) {
    size = (int)listBuffer[i];
    pos++;
    if (pos == 6) {
      /* get manufacturer at position 6 in list */
      size = (size > maxSize-1) ? maxSize : size;
      memcpy(str, &listBuffer[i+1], size);
      str[size+1] = 0;
    }
    i += listBuffer[i]+1;
  }
}

void smlOBISWh(double &wh) {
  unsigned char i = 0, pos = 0, size = 0;
  unsigned char scaler = 0;
  wh = -1; /* unknown or error */
  double l = 0;
  while (i < listPos) {
    pos++;
    if (pos == 4 && listBuffer[i+1] != SML_WATT_HOUR) {
      /* if unit at position 4 is not 0x1e (wh) return unknown */
      return;
    }
    if (pos == 5) {
      scaler = listBuffer[i+1];
    }
    if (pos == 6) {
      size = (int)listBuffer[i];
      if (size == 4) {
        /* 32 bit */
        l = (long int)listBuffer[i+1] << 24 
          | (long int)listBuffer[i+2] << 16 
          | (long int)listBuffer[i+3] << 8 
          | listBuffer[i+4]; 
      }
      if (size == 5) {
        /* 40 bit */
        l = (long long int)listBuffer[i+1] << 32 
          | (long int)listBuffer[i+2] << 24 
          | (long int)listBuffer[i+3] << 16 
          | (long int)listBuffer[i+4] << 8 
          | listBuffer[i+5]; 
      }
      if (size == 8) {
        /* 56 bit */
        l = 
          (long long int)listBuffer[i+1] << 56
          | (long long int)listBuffer[i+2] << 48 
          | (long long int)listBuffer[i+3] << 40 
          | (long long int)listBuffer[i+4] << 32 
          | (long int)listBuffer[i+5] << 24 
          | (long int)listBuffer[i+6] << 16 
          | (long int)listBuffer[i+7] << 8 
          | listBuffer[i+8];
      }
      switch (scaler) {
        case 0xFF: wh = l / 10; break;
        case 0xFE: wh = l / 100; break;
        case 0xFD: wh = l / 1000; break;
        case 0xFC: wh = l / 10000; break;
        case 0xFB: wh = l / 100000; break;
        case 0xFA: wh = l / 1000000; break;
        case 0x01: wh = l / 10; break;
        case 0x02: wh = l / 100; break;
        case 0x03: wh = l / 1000; break;
        default: wh = -3;
      }
    }
    i += listBuffer[i]+1;
  }
}
