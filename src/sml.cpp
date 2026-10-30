#include <stdio.h>
#include <string.h>

#include "sml.h"
#include "smlCrcTable.h"

#ifdef SML_DEBUG

#ifdef SML_NATIVE
#define SML_LOG(...)                                                           \
  do {                                                                         \
    printf(__VA_ARGS__);                                                       \
  } while (0)
#define SML_TREELOG(level, ...)                                                \
  do {                                                                         \
    printf("%.*s", level, "        ");                                         \
    printf(__VA_ARGS__);                                                       \
  } while (0)
#elif ARDUINO
#include <Arduino.h>
#define SML_LOG(...)                                                           \
  do {                                                                         \
    sprintf(logBuff, __VA_ARGS__);                                             \
    Serial.print(logBuff);                                                     \
  } while (0)
#define SML_TREELOG(level, ...)                                                \
  do {                                                                         \
    sprintf(logBuff, __VA_ARGS__);                                             \
    Serial.print(logBuff);                                                     \
  } while (0)
#endif

#else
#define SML_LOG(...)                                                           \
  do {                                                                         \
  } while (0)
#define SML_TREELOG(level, ...)                                                \
  do {                                                                         \
  } while (0)
#endif

// Define a single static instance to maintain operation of legacy C style global functions
// (e.g. see non-member functions smlState, smlOBISCheck, etc.)
SmlParser SmlParser1;


SmlParser::SmlParser()
{
   reset();
}

void SmlParser::reset()
{
   currentState = SML_START;
   nodes[0] = 0;
   currentLevel = 0;
   crc = 0xFFFF;
   crcMine = 0xFFFF;
   crcReceived = 0x0000;
   len = 4;
   listPos = 0;
}

void SmlParser::crc16(unsigned char byte)
{
  // low nibble first, then high nibble
#ifdef ARDUINO
  crc = pgm_read_word_near(&smlCrcTable[(byte ^ crc) & 0x0f]) ^ (crc >> 4);
  crc = pgm_read_word_near(&smlCrcTable[((byte >> 4) ^ crc) & 0x0f]) ^ (crc >> 4);
#else
  crc = smlCrcTable[(byte ^ crc) & 0x0f] ^ (crc >> 4);
  crc = smlCrcTable[((byte >> 4) ^ crc) & 0x0f] ^ (crc >> 4);
#endif
}

void SmlParser::setState(sml_states_t state, int byteLen)
{
  currentState = state;
  len = byteLen;
}

void SmlParser::pushListBuffer(unsigned char byte)
{
  if (listPos < MAX_LIST_SIZE) {
    listBuffer[listPos++] = byte;
  }
}

void SmlParser::reduceList()
{
  if (currentLevel >= 0 && nodes[currentLevel] > 0)
    nodes[currentLevel]--;
}

void SmlParser::smlNewList(unsigned char size)
{
  reduceList();
  if (currentLevel < MAX_TREE_SIZE - 1)
    currentLevel++;
  nodes[currentLevel] = size;
  SML_TREELOG(currentLevel, "LISTSTART on level %i with %i nodes\n",
              currentLevel, size);
  setState(SML_LISTSTART, size);
  // @todo workaround for lists inside obis lists
  if (size > 5) {
    listPos = 0;
    memset(listBuffer, '\0', MAX_LIST_SIZE);
  }
  else {
    pushListBuffer(size);
    pushListBuffer(currentState);
  }
}

void SmlParser::checkMagicByte(unsigned char byte)
{
  unsigned int size = 0;
  while (currentLevel > 0 && nodes[currentLevel] == 0) {
    /* go back in tree if no nodes remaining */
    SML_TREELOG(currentLevel, "back to previous list\n");
    currentLevel--;
  }
  if (byte > 0x70 && byte <= 0x7F) {
    /* new list */
    size = byte & 0x0F;
    smlNewList(size);
  }
  else if (byte >= 0x01 && byte <= 0x6F && nodes[currentLevel] > 0) {
    if (byte == 0x01) {
      /* no data, get next */
      SML_TREELOG(currentLevel, " Data %i (empty)\n", nodes[currentLevel]);
      pushListBuffer(0);
      pushListBuffer(currentState);
      if (nodes[currentLevel] == 1) {
        setState(SML_LISTEND, 1);
        SML_TREELOG(currentLevel, "LISTEND\n");
      }
      else {
        setState(SML_NEXT, 1);
      }
    }
    else {
      size = (byte & 0x0F) - 1;
      setState(SML_DATA, size);
      if ((byte & 0xF0) == 0x50) {
        setState(SML_DATA_SIGNED_INT, size);
      }
      else if ((byte & 0xF0) == 0x60) {
        setState(SML_DATA_UNSIGNED_INT, size);
      }
      else if ((byte & 0xF0) == 0x00) {
        setState(SML_DATA_OCTET_STRING, size);
      }
      SML_TREELOG(currentLevel,
                  " Data %i (length = %i%s): ", nodes[currentLevel], size,
                  (currentState == SML_DATA_SIGNED_INT)     ? ", signed int"
                  : (currentState == SML_DATA_UNSIGNED_INT) ? ", unsigned int"
                  : (currentState == SML_DATA_OCTET_STRING) ? ", octet string"
                                                            : "");
      pushListBuffer(size);
      pushListBuffer(currentState);
    }
    reduceList();
  }
  else if (byte == 0x00) {
    /* end of block */
    reduceList();
    SML_TREELOG(currentLevel, "End of block at level %i\n", currentLevel);
    if (currentLevel == 0) {
      setState(SML_NEXT, 1);
    }
    else {
      setState(SML_BLOCKEND, 1);
    }
  }
  else if (byte & 0x80) {
    // MSB bit is set, another TL byte will follow
    if (byte >= 0x80 && byte <= 0x8F) {
      // Datatype Octet String
      setState(SML_HDATA, (byte & 0x0F) << 4);
      reduceList();
    }
    else if (byte >= 0xF0 && byte <= 0xFF) {
      /* Datatype List of ...*/
      setState(SML_LISTEXTENDED, (byte & 0x0F) << 4);
    }
  }
  else if (byte == 0x1B && currentLevel == 0) {
    /* end sequence */
    setState(SML_END, 3);
  }
  else {
    /* Unexpected Byte */
    SML_TREELOG(currentLevel,
                "UNEXPECTED magicbyte >%02X< at currentLevel %i\n", byte,
                currentLevel);
    setState(SML_UNEXPECTED, 4);
  }
}

sml_states_t smlState(unsigned char currentByte)
{
  return SmlParser1.smlState(currentByte);
}

sml_states_t SmlParser::smlState(unsigned char currentByte)
{
  unsigned char size;
  if (len > 0)
    len--;
  crc16(currentByte);
  switch (currentState) {
  case SML_UNEXPECTED:
  case SML_CHECKSUM_ERROR:
  case SML_FINAL:
  case SML_START:
    currentState = SML_START;
    currentLevel = 0; // Reset current level at the begin of a new transmission
                      // to prevent problems
    if (currentByte != 0x1b)
      setState(SML_UNEXPECTED, 4);
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
    if (currentByte != 0x01)
      setState(SML_UNEXPECTED, 4);
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
      }
      else {
        setState(SML_CHECKSUM_ERROR, 4);
      }
      crc = 0xFFFF;
      crcReceived = 0x000; /* reset CRC */
    }
    break;
  case SML_HDATA:
    size = len + currentByte - 1;
    setState(SML_DATA, size);
    pushListBuffer(size);
    pushListBuffer(currentState);
    SML_TREELOG(currentLevel, " Data (length = %i): ", size);
    break;
  case SML_LISTEXTENDED:
    /* len holds the high nibble shifted by 4 and was decremented for this
       byte. Unlike for octet strings, a list length does not include the TL
       bytes. */
    size = len + 1 + (currentByte & 0x0F);
    SML_TREELOG(currentLevel, "Extended List with Size=%i\n", size);
    smlNewList(size);
    break;
  case SML_DATA:
  case SML_DATA_SIGNED_INT:
  case SML_DATA_UNSIGNED_INT:
  case SML_DATA_OCTET_STRING:
    SML_LOG("%02X ", currentByte);
    pushListBuffer(currentByte);
    if (nodes[currentLevel] == 0 && len == 0) {
      SML_LOG("\n");
      SML_TREELOG(currentLevel, "LISTEND on level %i\n", currentLevel);
      currentState = SML_LISTEND;
    }
    else if (len == 0) {
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

bool smlOBISCheck(const unsigned char *obis)
{
  return SmlParser1.smlOBISCheck(obis);
}

bool SmlParser::smlOBISCheck(const unsigned char *obis)
{
  return (memcmp(obis, &listBuffer[2], 6) == 0);
}

bool smlOBISEntry(sml_obis_entry_t &entry)
{
  return SmlParser1.smlOBISEntry(entry);
}

bool SmlParser::smlOBISEntry(sml_obis_entry_t &entry)
{
  /* An SML_ListEntry has 7 elements: objName (OBIS), status, valTime, unit,
     scaler, value and valueSignature. The buffer keeps them as
     size + type + data, a nested list (e.g. valTime) as size + SML_LISTSTART
     followed by its elements. */
  int i = 0, pos = 0, skip = 0, size, y;
  sml_states_t type;
  memset(&entry, 0, sizeof(entry));
  while (i + 2 <= listPos) {
    size = listBuffer[i++];
    type = (sml_states_t)listBuffer[i++];
    if (type == SML_LISTSTART) {
      /* a nested list counts as a single element, skip its children */
      if (skip > 0)
        skip--;
      else
        pos++;
      skip += size;
      continue;
    }
    if (i + size > listPos) return false; /* truncated */
    if (skip > 0) {
      skip--;
      i += size;
      continue;
    }
    pos++;
    if (pos == 1) {
      if (size != 6 || type != SML_DATA_OCTET_STRING) return false;
      memcpy(entry.obis, &listBuffer[i], 6);
    }
    else if (pos == 4 && size > 0) {
      entry.unit = listBuffer[i + size - 1];
    }
    else if (pos == 5 && size > 0) {
      entry.scaler = (signed char)listBuffer[i + size - 1];
    }
    else if (pos == 6) {
      entry.type = (size > 0) ? type : SML_DATA;
      entry.bytes = &listBuffer[i];
      entry.length = size;
      if ((type == SML_DATA_SIGNED_INT || type == SML_DATA_UNSIGNED_INT) &&
          size > 0 && size <= 8) {
        /* Accumulate in an unsigned type: left-shifting a negative signed value
           is undefined behaviour. The two's-complement bit pattern is unchanged. */
        unsigned long long v =
            (type == SML_DATA_SIGNED_INT && (listBuffer[i] & (1 << 7))) ? ~0ULL : 0;
        for (y = 0; y < size; y++) {
          v = (v << 8) | listBuffer[i + y];
        }
        entry.value = (long long)v;
      }
    }
    i += size;
  }
  return pos == 7 && skip == 0;
}

int smlOBISToString(const unsigned char *obis, char *str, int maxSize)
{
  return snprintf(str, maxSize, "%d-%d:%d.%d.%d*%d", obis[0], obis[1],
                  obis[2], obis[3], obis[4], obis[5]);
}

bool smlOBISFromString(const char *str, unsigned char *obis)
{
  /* A-B:C.D.E*F, F is optional */
  static const char separators[] = "-:..*";
  int n = 0, val;
  while (n < 6) {
    if (*str < '0' || *str > '9') return false;
    for (val = 0; *str >= '0' && *str <= '9'; str++) {
      val = val * 10 + (*str - '0');
      if (val > 255) return false;
    }
    obis[n++] = (unsigned char)val;
    if (*str == '\0') break;
    if (n == 6 || *str != separators[n - 1]) return false;
    str++;
  }
  if (n == 5) {
    obis[5] = 0xff;
    return true;
  }
  return n == 6;
}

void smlOBISManufacturer(unsigned char *str, int maxSize)
{
  SmlParser1.smlOBISManufacturer(str, maxSize);
}

void SmlParser::smlOBISManufacturer(unsigned char *str, int maxSize)
{
  int i = 0, pos = 0, size = 0;
  if (maxSize < 1) return;
  while (i < listPos) {
    size = (int)listBuffer[i];
    i++;
    pos++;
    if (pos == 6) {
      /* get manufacturer at position 6 in list */
      if (size > maxSize - 1) size = maxSize - 1; /* leave room for the terminator */
      if (i + 1 + size > listPos) size = (i + 1 < listPos) ? (listPos - (i + 1)) : 0; /* stay in buffer */
      if (size > 0) memcpy(str, &listBuffer[i + 1], size); /* i + 1 may be past the buffer */
      str[size] = 0;
      break;
    }
    i += size + 1;
  }
}

int smlOBISServerId(unsigned char* bytes, int maxSize)
{
  return SmlParser1.smlOBISServerId(bytes, maxSize);
}

int SmlParser::smlOBISServerId(unsigned char* bytes, int maxSize)
{
    int i = 0, pos = 0, size = 0;
    while (i < listPos) {
        size = (int)listBuffer[i];
        i++;
        pos++;
        if (pos == 6) {
            /* get serverId at position 6 in list */
            if (size > maxSize) size = (maxSize > 0) ? maxSize : 0;
            if (i + 1 + size > listPos) size = (i + 1 < listPos) ? (listPos - (i + 1)) : 0; /* stay in buffer */
            if (size > 0) memcpy(bytes, &listBuffer[i + 1], size); /* i + 1 may be past the buffer */
            return size;
        }
        i += size + 1;
    }
    return 0;
}

void smlPow(double &val, signed char &scaler)
{
  if (scaler < 0) {
    while (scaler++) {
      val /= 10;
    }
  }
  else {
    while (scaler--) {
      val *= 10;
    }
  }
}

void smlOBISByUnit(long long int &val, signed char &scaler, sml_units_t unit)
{
  SmlParser1.smlOBISByUnit(val, scaler, unit);
}

void SmlParser::smlOBISByUnit(long long int &val, signed char &scaler, sml_units_t unit)
{
  unsigned char i = 0, pos = 0, size = 0, y = 0, skip = 0;
  sml_states_t type;
  val = -1; /* unknown or error */
  scaler = 0; /* do not leave a stale or uninitialised scaler on error */
  while (i < listPos) {
    pos++;
    if (i + 2 > listPos) return; /* need the size and type bytes */
    size = (int)listBuffer[i++];
    type = (sml_states_t)listBuffer[i++];
    if (type == SML_LISTSTART && size > 0) {
      // skip a list inside an obis list
      skip = size;
      while (skip > 0) {
        if (i + 2 > listPos) return;
        size = (int)listBuffer[i++];
        type = (sml_states_t)listBuffer[i++];
        if (i + size > listPos) return; /* do not skip past the buffer */
        i += size;
        skip--;
      }
      size = 0;
    }
    if (pos == 4) {
      if (i >= listPos) return;
      if (listBuffer[i] != unit) {
        /* return unknown (-1) if unit does not match */
        return;
      }
    }
    if (pos == 5) {
      if (i >= listPos) return;
      scaler = listBuffer[i];
    }
    if (pos == 6) {
      if (size < 1 || i + size > listPos) return; /* value must fit in the buffer */
      y = size;
      // initialize 64bit integer based on MSB from received value, unsigned
      // because left-shifting a negative signed value is undefined behaviour
      unsigned long long v =
          (type == SML_DATA_SIGNED_INT && (listBuffer[i] & (1 << 7))) ? ~0ULL : 0;
      for (y = 0; y < size; y++) {
        // left shift received bytes to 64 bit integer
        v = (v << 8) | listBuffer[i + y];
      }
      val = (long long)v;
      return;
    }
    if (i + size > listPos) return; /* next entry must stay in the buffer */
    i += size;
  }
}

void smlOBISWh(double &wh)
{
  SmlParser1.smlOBISWh(wh);
}

void SmlParser::smlOBISWh(double &wh)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_WATT_HOUR);
  wh = val;
  smlPow(wh, sc);
}

void smlOBISW(double &w)
{
  SmlParser1.smlOBISW(w);
}

void SmlParser::smlOBISW(double &w)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_WATT);
  w = val;
  smlPow(w, sc);
}

void smlOBISVolt(double &v)
{
  SmlParser1.smlOBISVolt(v);
}

void SmlParser::smlOBISVolt(double &v)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_VOLT);
  v = val;
  smlPow(v, sc);
}

void smlOBISAmpere(double &a)
{
  SmlParser1.smlOBISAmpere(a);
}

void SmlParser::smlOBISAmpere(double &a)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_AMPERE);
  a = val;
  smlPow(a, sc);
}

void smlOBISHertz(double &h)
{
  SmlParser1.smlOBISHertz(h);
}

void SmlParser::smlOBISHertz(double &h)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_HERTZ);
  h = val;
  smlPow(h, sc);
}

void smlOBISDegree(double &d)
{
  SmlParser1.smlOBISDegree(d);
}

void SmlParser::smlOBISDegree(double &d)
{
  long long int val;
  smlOBISByUnit(val, sc, SML_DEGREE);
  d = val;
  smlPow(d, sc);
}
