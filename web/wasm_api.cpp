#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sml.h"

/*
 * Thin wrapper to run the parser in the browser (see web/README.md).
 * web_parse() feeds a buffer byte by byte into a fresh parser and returns the
 * events as JSON lines, the tree log of SML_DEBUG goes to stdout.
 */

static char *out = 0;
static int outLen = 0, outSize = 0;

static void emit(const char *fmt, ...)
{
  va_list args;
  int n;
  va_start(args, fmt);
  n = vsnprintf(0, 0, fmt, args);
  va_end(args);
  if (outLen + n + 1 > outSize) {
    outSize = (outLen + n + 1) * 2;
    out = (char *)realloc(out, outSize);
  }
  va_start(args, fmt);
  vsnprintf(out + outLen, outSize - outLen, fmt, args);
  va_end(args);
  outLen += n;
}

static void emitHex(const unsigned char *bytes, int length)
{
  int i;
  for (i = 0; i < length; i++) {
    emit("%02x", bytes[i]);
  }
}

extern "C" {

const char *web_parse(const unsigned char *data, int length)
{
  SmlParser parser;
  sml_obis_entry_t entry;
  sml_states_t s, prev = SML_START;
  char obis[24];
  int i;

  outLen = 0;
  emit("");
  for (i = 0; i < length; i++) {
    s = parser.smlState(data[i]);
    if (s == SML_LISTEND && parser.smlOBISEntry(entry)) {
      smlOBISToString(entry.obis, obis, sizeof(obis));
      emit("{\"event\":\"entry\",\"offset\":%d,\"obis\":\"%s\",\"unit\":%d,"
           "\"scaler\":%d,\"type\":%d,\"value\":\"%lld\",\"bytes\":\"",
           i, obis, entry.unit, entry.scaler, entry.type, entry.value);
      emitHex(entry.bytes, entry.length);
      emit("\"}\n");
    }
    else if (s == SML_FINAL && prev != SML_FINAL) {
      emit("{\"event\":\"final\",\"offset\":%d}\n", i);
    }
    else if (s == SML_CHECKSUM_ERROR && prev != SML_CHECKSUM_ERROR) {
      emit("{\"event\":\"checksum_error\",\"offset\":%d}\n", i);
    }
    else if (s == SML_UNEXPECTED && prev != SML_UNEXPECTED &&
             prev != SML_START) {
      /* only report errors inside a message, not garbage between messages */
      emit("{\"event\":\"unexpected\",\"offset\":%d,\"byte\":%d}\n", i,
           data[i]);
    }
    else if (s == SML_VERSION && prev != SML_VERSION) {
      emit("{\"event\":\"start\",\"offset\":%d}\n", i - 3);
    }
    prev = s;
  }
  fflush(stdout);
  return out;
}
}
