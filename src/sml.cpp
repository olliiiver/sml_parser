
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "smlCrcTable.h"
#include "sml.h"

#ifdef DEBUG_NATIVE
#define SML_LOG(...) do { printf(  __VA_ARGS__); } while (0)
#define SML_TREELOG(level, ...) do { printf("%.*s", level, "        "); printf(  __VA_ARGS__); } while (0)
#else
#define SML_LOG(...) 
#define SML_TREELOG(level, ...) 
#endif 

#define MAX_LIST_SIZE 80
#define MAX_TREE_SIZE 5

static sml_states_t currentState = SML_START;
static char nodes[MAX_TREE_SIZE];
static int currentLevel = 0;
static unsigned short crc = 0xFFFF;
static unsigned short crcMine = 0xFFFF;
static unsigned short crcReceived = 0x0000;
static int len = 4;
static unsigned char listBuffer[MAX_LIST_SIZE]; /* keeps a two dimensional list as length + data array */
static int listPos = 0;

void crc16(unsigned char byte) {
  crc = smlCrcTable[(byte ^ crc) & 0xff] ^ (crc >> 8 & 0xff);
}

void setState (sml_states_t state, int byteLen) {
  currentState = state;
  len = byteLen;
}

void checkMagicByte (unsigned char byte) {
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
    nodes[currentLevel]--; /* reduce previous list */
    currentLevel++;
    nodes[currentLevel] = size;
    SML_TREELOG(currentLevel, "LISTSTART with %i nodes\n", size);
    setState(SML_LISTSTART, size);
    listPos = 0;
  } else if (byte >= 0x01 && byte <= 0x6F && nodes[currentLevel] > 0) {
    if (byte == 0x01) {
      /* no data, get next */
      SML_TREELOG(currentLevel, " Data %i (empty)\n", nodes[currentLevel]);
      listBuffer[listPos++] = 0;
      if (nodes[currentLevel] == 1) {
        setState(SML_LISTEND, 1);
        SML_TREELOG(currentLevel, "LISTEND\n");
      } else {
        setState(SML_NEXT, 1); 
      }
    } else {
      size = (byte & 0x0F) - 1;
      SML_TREELOG(currentLevel, " Data %i (length = %i): ", nodes[currentLevel], size);
      listBuffer[listPos++] = size;
      setState(SML_DATA, size);
    }
    nodes[currentLevel]--; /* reduce current list */
  } else if (byte == 0x00) {
    /* end of block */
    nodes[currentLevel]--; /* reduce current list (end of block) */
    SML_TREELOG(currentLevel, "End of block %i\n", currentLevel);
    if (currentLevel == 0) {
      setState(SML_END, 4);
    } else {
      setState(SML_BLOCKEND, 1);
    }
  } else if (byte >= 0x80 && byte <= 0x8F) {
    setState(SML_HDATA, (byte & 0x0F) << 4);
  } else {
    /* Unexpected Byte */
    SML_TREELOG(currentLevel, "UNEXPECTED %i >%x<\n", nodes[currentLevel], byte);
    setState(SML_UNEXPECTED, 4);
  }
}

sml_states_t smlState (unsigned char c) {
  int size;
  if (len > 0) len--;
  crc16(c);
  switch (currentState) {
    case SML_UNEXPECTED:
    case SML_CHECKSUM_ERROR:
    case SML_FINAL:
    case SML_START:
      currentState = SML_START;
      if (c != 0x1b) setState(SML_UNEXPECTED, 4);
      if (len == 0) {
        SML_TREELOG(0, "START\n");
        setState(SML_VERSION, 4);
      }
    break;
    case SML_VERSION:
      if (c != 0x01) setState(SML_UNEXPECTED, 4);
      if (len == 0) {
        setState(SML_BLOCKSTART, 1);
      }
    break;
    case SML_END:
      if (c != 0x1b) setState(SML_UNEXPECTED, 4);
      if (len == 0) {
        setState(SML_CHECKSUM, 4);
      }
    break;
    case SML_CHECKSUM:
      // SML_LOG("CHECK: %02X\n", c);
      if (len == 2) {
        crcMine = crc ^ 0xFFFF;
      }
      if (len == 1) {
        crcReceived += c;
      }
      if (len == 0) {
        crcReceived = crcReceived | (c << 8);
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
      size = len + c-1;
      setState(SML_DATA, size);
      listBuffer[listPos++] = size;
      SML_TREELOG(currentLevel, " Data (length = %i): ", size);
    break;
    case SML_DATA:
      SML_LOG("%02X ", c);
      listBuffer[listPos++] = c;
      if (nodes[currentLevel] == 0 && len == 0) {
        SML_LOG("\n");
        SML_TREELOG(currentLevel, "LISTEND\n");
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
      checkMagicByte(c);
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
  int i = 0, pos = 0, size = 0;
  char scaler = 0;
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
      if (size == 5) {
        /* 40 bit */
        l = (long int)listBuffer[i+1] << 32 
          | listBuffer[i+2] << 24 
          | listBuffer[i+3] << 16 
          | listBuffer[i+4] << 8 
          | listBuffer[i+5]; 
      }
      if (size == 8) {
        /* 56 bit */
        l = 
          (long int)listBuffer[i+1] << 56
          | (long int)listBuffer[i+2] << 48 
          | (long int)listBuffer[i+3] << 40 
          | (long int)listBuffer[i+4] << 32 
          | listBuffer[i+5] << 24 
          | listBuffer[i+6] << 16 
          | listBuffer[i+7] << 8 
          | listBuffer[i+8];
      }
      wh = l * pow(10, scaler);
    }
    i += listBuffer[i]+1;
  }
}



