/*
 * Mutation and generation fuzzer for the SML parser, see README.md.
 *
 *   fuzz <seconds> <seed> <input file> <seed dumps...>
 *     Fuzz for the given time. Before each run the input is written to
 *     <input file>, so after a crash it holds the input that caused it.
 *   fuzz --replay <files...>
 *     Run the given inputs once each, e.g. to reproduce a crash.
 *
 * Every input is fed byte by byte into a fresh heap SmlParser and into the
 * global legacy API. After a list or value ends, all public accessors are
 * called. Problems are reported by ASan/UBSan or by the checks below, all of
 * which abort.
 */
#define private public /* to check the internal invariants */
#include "sml.h"
#undef private
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

typedef std::vector<unsigned char> Bytes;

static uint64_t rs;
static uint32_t rnd()
{
  rs ^= rs << 13;
  rs ^= rs >> 7;
  rs ^= rs << 17;
  return (uint32_t)rs;
}
static uint32_t R(uint32_t n) { return n ? rnd() % n : 0; }

/* CRC-16/X-25 as used by SML */
static uint16_t crc16(const Bytes &b, size_t from, size_t to)
{
  uint16_t c = 0xffff;
  for (size_t i = from; i < to; i++) {
    c ^= b[i];
    for (int k = 0; k < 8; k++)
      c = (c & 1) ? (c >> 1) ^ 0x8408 : c >> 1;
  }
  return c ^ 0xffff;
}

static bool isEscape(const Bytes &b, size_t i, unsigned char next)
{
  return i + 4 < b.size() && b[i] == 0x1b && b[i + 1] == 0x1b &&
         b[i + 2] == 0x1b && b[i + 3] == 0x1b && b[i + 4] == next;
}

/* Wrap a message body into a transport frame with a valid checksum. */
static Bytes frame(const Bytes &body)
{
  Bytes m = {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
  m.insert(m.end(), body.begin(), body.end());
  int pad = 0;
  while (m.size() % 4) {
    m.push_back(0);
    pad++;
  }
  const unsigned char end[] = {0x1b, 0x1b, 0x1b, 0x1b, 0x1a};
  m.insert(m.end(), end, end + 5);
  m.push_back((unsigned char)pad);
  uint16_t c = crc16(m, 0, m.size());
  m.push_back(c & 0xff);
  m.push_back(c >> 8);
  return m;
}

/* Recompute the checksum of every complete frame in place, so mutated
   messages still reach SML_FINAL. */
static void fixCrcs(Bytes &m)
{
  size_t start = 0;
  for (size_t i = 0; i + 8 <= m.size(); i++) {
    if (isEscape(m, i, 0x01)) {
      start = i;
      i += 7;
    }
    else if (isEscape(m, i, 0x1a)) {
      uint16_t c = crc16(m, start, i + 6);
      m[i + 6] = c & 0xff;
      m[i + 7] = c >> 8;
      i += 7;
    }
  }
}

/* Split a recorded dump into single frames, which mutate better. */
static void addFrames(std::vector<Bytes> &corpus, const Bytes &dump)
{
  size_t start = 0;
  bool inFrame = false;
  for (size_t i = 0; i + 8 <= dump.size(); i++) {
    if (isEscape(dump, i, 0x01)) {
      start = i;
      inFrame = true;
      i += 7;
    }
    else if (inFrame && isEscape(dump, i, 0x1a)) {
      corpus.push_back(Bytes(dump.begin() + start, dump.begin() + i + 8));
      inFrame = false;
      i += 7;
    }
  }
}

static const unsigned char interesting[] = {
    0x00, 0x01, 0x02, 0x0f, 0x1b, 0x1a, 0x42, 0x50, 0x51, 0x52, 0x55, 0x59,
    0x5f, 0x60, 0x62, 0x65, 0x69, 0x6f, 0x70, 0x71, 0x76, 0x77, 0x7f, 0x80,
    0x81, 0x83, 0x8f, 0x90, 0xf0, 0xf1, 0xff, 0x1e, 0x23, 0x21, 0x2c};

/* ---- structured generator ---- */

static const size_t MAX_GENERATED = 1500;

static void genElem(Bytes &b, int depth);

static void genInt(Bytes &b)
{
  int size = R(10) ? 1 + R(8) : R(16); /* sometimes zero or oversized */
  b.push_back((R(2) ? 0x50 : 0x60) | ((size + 1) & 0x0f));
  for (int i = 0; i < size; i++)
    b.push_back(R(3) ? (unsigned char)rnd() : (R(2) ? 0xff : 0x80));
}

static void genOctet(Bytes &b)
{
  if (R(4)) {
    int size = R(15);
    b.push_back((size + 1) & 0x0f);
    for (int i = 0; i < size; i++)
      b.push_back(rnd());
  }
  else {
    /* two byte TL, the length includes both TL bytes */
    int total = 2 + R(254);
    b.push_back(0x80 | ((total >> 4) & 0x0f));
    b.push_back(total & 0x0f);
    for (int i = 0; i < total - 2; i++)
      b.push_back(rnd());
  }
}

static void genList(Bytes &b, int depth)
{
  int n;
  if (R(8)) {
    n = R(16);
    b.push_back(0x70 | n);
  }
  else {
    /* two byte TL */
    n = 16 + R(40);
    b.push_back(0xf0 | ((n >> 4) & 0x0f));
    b.push_back(n & 0x0f);
  }
  for (int i = 0; i < n; i++)
    genElem(b, depth + 1);
}

/* SML_ListEntry: OBIS, status, valTime, unit, scaler, value, signature */
static void genEntry(Bytes &b, int depth)
{
  static const unsigned char units[] = {0x1e, 0x1b, 0x23, 0x21,
                                        0x2c, 0x08, 0x00, 0xff};
  static const unsigned char obis180[] = {1, 0, 1, 8, 0, 0xff};
  b.push_back(R(10) ? 0x77 : (0x70 | R(16)));
  b.push_back(0x07);
  if (R(2))
    b.insert(b.end(), obis180, obis180 + 6);
  else
    for (int i = 0; i < 6; i++)
      b.push_back(rnd());
  if (R(4)) b.push_back(0x01); else genElem(b, depth + 1);
  if (R(4)) b.push_back(0x01); else genElem(b, depth + 1);
  if (R(6)) {
    b.push_back(0x62);
    b.push_back(units[R(sizeof(units))]);
  }
  else
    genElem(b, depth + 1);
  if (R(6)) {
    b.push_back(0x52);
    b.push_back(rnd());
  }
  else
    genElem(b, depth + 1);
  if (R(5)) genInt(b); else genElem(b, depth + 1);
  b.push_back(0x01);
}

static void genElem(Bytes &b, int depth)
{
  if (depth > 20 || b.size() > MAX_GENERATED) {
    b.push_back(0x01);
    return;
  }
  switch (R(12)) {
  case 0: case 1: genInt(b); break;
  case 2: case 3: genOctet(b); break;
  case 4: case 5: genList(b, depth); break;
  case 6: case 7: genEntry(b, depth); break;
  case 8: b.push_back(0x01); break;
  case 9: b.push_back(0x42); b.push_back(rnd() & 1); break;
  case 10: b.push_back(0x00); break;
  default: b.push_back(rnd()); break;
  }
}

/* One to three getListResponse messages */
static Bytes genMessage()
{
  Bytes b;
  int msgs = 1 + R(3);
  for (int m = 0; m < msgs; m++) {
    const unsigned char head[] = {0x76, 0x05};
    b.insert(b.end(), head, head + 2);
    for (int i = 0; i < 4; i++) b.push_back(rnd()); /* transaction id */
    const unsigned char body[] = {0x62, 0x00, 0x62, 0x00, 0x72, 0x63, 0x07,
                                  0x01, 0x77, 0x01, 0x0b};
    b.insert(b.end(), body, body + sizeof(body));
    for (int i = 0; i < 10; i++) b.push_back(rnd()); /* server id */
    const unsigned char time[] = {0x01, 0x72, 0x62, 0x01, 0x65};
    b.insert(b.end(), time, time + sizeof(time));
    for (int i = 0; i < 4; i++) b.push_back(rnd());
    int n = R(16);
    b.push_back(0x70 | n); /* valList */
    for (int i = 0; i < n; i++) {
      if (R(4)) genEntry(b, 2); else genElem(b, 2);
    }
    const unsigned char tail[] = {0x01, 0x01, 0x63};
    b.insert(b.end(), tail, tail + sizeof(tail));
    b.push_back(rnd()); /* message crc, not checked by the parser */
    b.push_back(rnd());
    b.push_back(0x00);
  }
  return b;
}

static Bytes generate()
{
  Bytes b;
  switch (R(4)) {
  case 0:
    genElem(b, 0);
    break;
  case 1: { /* deep nesting */
    int n = 1 + R(20);
    for (int i = 0; i < n; i++) b.push_back(0x70 | (1 + R(15)));
    for (int i = 0; i < n * 3; i++) genElem(b, 15);
    break;
  }
  default:
    b = genMessage();
  }
  return frame(b);
}

/* ---- mutator ---- */

static void mutate(Bytes &b, const std::vector<Bytes> &corpus)
{
  int n = 1 + R(8);
  for (int k = 0; k < n; k++) {
    size_t sz = b.size();
    switch (R(9)) {
    case 0: /* flip a bit */
      if (sz) b[R(sz)] ^= 1 << R(8);
      break;
    case 1: /* random byte */
      if (sz) b[R(sz)] = rnd();
      break;
    case 2: /* interesting byte */
      if (sz) b[R(sz)] = interesting[R(sizeof(interesting))];
      break;
    case 3: /* insert interesting byte */
      b.insert(b.begin() + R(sz + 1), interesting[R(sizeof(interesting))]);
      break;
    case 4: /* erase a range */
      if (sz) {
        size_t p = R(sz), l = 1 + R(16);
        if (p + l > sz) l = sz - p;
        b.erase(b.begin() + p, b.begin() + p + l);
      }
      break;
    case 5: /* duplicate a range */
      if (sz) {
        size_t p = R(sz), l = 1 + R(32);
        if (p + l > sz) l = sz - p;
        Bytes c(b.begin() + p, b.begin() + p + l);
        b.insert(b.begin() + R(sz + 1), c.begin(), c.end());
      }
      break;
    case 6: { /* splice in a range of another input */
      const Bytes &o = corpus[R(corpus.size())];
      if (o.empty()) break;
      size_t p = R(o.size()), l = 1 + R(64);
      if (p + l > o.size()) l = o.size() - p;
      b.insert(b.begin() + R(sz + 1), o.begin() + p, o.begin() + p + l);
      break;
    }
    case 7: { /* insert a generated element */
      Bytes g;
      genElem(g, 0);
      b.insert(b.begin() + R(sz + 1), g.begin(), g.end());
      break;
    }
    case 8: { /* insert random bytes */
      size_t p = R(sz + 1);
      int l = 1 + R(64);
      for (int i = 0; i < l; i++) b.insert(b.begin() + p, (unsigned char)rnd());
      break;
    }
    }
  }
  if (b.size() > 4096) b.resize(4096);
  if (R(2)) fixCrcs(b);
}

/* ---- exercising the API ---- */

static volatile unsigned long long sink; /* keeps reads from being optimised away */
static unsigned long finals, entries, unitHits;

static void fail(const char *msg)
{
  fprintf(stderr, "fuzz check failed: %s\n", msg);
  abort();
}

static void touch(const void *p, int n)
{
  const unsigned char *c = (const unsigned char *)p;
  for (int i = 0; i < n; i++) sink += c[i];
}

static void checkInvariants(SmlParser &p)
{
  if (p.listPos > SmlParser::MAX_LIST_SIZE) fail("listPos past listBuffer");
  if (p.currentLevel >= SmlParser::MAX_TREE_SIZE) fail("currentLevel past nodes");
  if ((unsigned)p.currentState > SML_DATA_OCTET_STRING) fail("invalid state");
}

static void exercise(SmlParser &p, sml_states_t s)
{
  static const sml_units_t units[] = {SML_WATT_HOUR, SML_WATT, SML_VOLT,
                                      SML_AMPERE, SML_HERTZ, SML_DEGREE,
                                      SML_COUNT, (sml_units_t)0};
  static const unsigned char obis[6] = {1, 0, 1, 8, 0, 0xff};
  if (s == SML_FINAL) finals++;
  sink += p.smlOBISCheck(obis);

  sml_obis_entry_t e;
  if (p.smlOBISEntry(e)) {
    entries++;
    touch(e.obis, 6);
    if (e.length) {
      if (!e.bytes) fail("entry has a length but no bytes");
      touch(e.bytes, e.length); /* must stay inside the parser buffer */
    }
    char str[40];
    unsigned char back[6];
    smlOBISToString(e.obis, str, sizeof(str));
    if (!smlOBISFromString(str, back) || memcmp(back, e.obis, 6))
      fail("OBIS string round trip");
  }
  for (unsigned u = 0; u < sizeof(units) / sizeof(units[0]); u++) {
    long long v = 0;
    signed char sc = 0;
    p.smlOBISByUnit(v, sc, units[u]);
    if (v != -1) unitHits++;
    sink += (unsigned long long)v + (unsigned long long)sc;
  }
  double d;
  p.smlOBISWh(d);
  p.smlOBISW(d);
  p.smlOBISVolt(d);
  p.smlOBISAmpere(d);
  p.smlOBISHertz(d);
  p.smlOBISDegree(d);
  sink += (unsigned long long)isnan(d);

  /* caller buffers have their exact size, so ASan sees any overflow */
  static const int sizes[] = {-5, 0, 1, 2, 5, 10, 79, 100};
  for (unsigned k = 0; k < sizeof(sizes) / sizeof(sizes[0]); k++) {
    int n = sizes[k];
    unsigned char *buf = (unsigned char *)malloc(n > 0 ? n : 1);
    p.smlOBISManufacturer(buf, n);
    int got = p.smlOBISServerId(buf, n);
    if (got < 0 || got > (n > 0 ? n : 0)) fail("server id length");
    free(buf);
  }
}

static void runInput(const Bytes &b)
{
  /* on the heap, which ASan fills with garbage, to expose uninitialised use */
  SmlParser *p = new SmlParser();
  for (size_t i = 0; i < b.size(); i++) {
    sml_states_t s = p->smlState(b[i]);
    checkInvariants(*p);
    if (s == SML_LISTEND || s == SML_DATAEND || s == SML_FINAL || R(256) == 0)
      exercise(*p, s);
    if (smlState(b[i]) == SML_LISTEND) {
      sml_obis_entry_t e;
      double wh;
      unsigned char m[5];
      sink += smlOBISEntry(e);
      smlOBISWh(wh);
      smlOBISManufacturer(m, sizeof(m));
    }
  }
  if (R(4) == 0) {
    p->reset();
    checkInvariants(*p);
  }
  delete p;
}

static void fuzzFromString()
{
  char s[24];
  int n = R(sizeof(s));
  for (int i = 0; i < n; i++)
    s[i] = R(2) ? "0123456789-:.*"[R(14)] : (char)rnd();
  s[n] = 0;
  char *h = (char *)malloc(n + 1); /* exact size, so overreads are caught */
  memcpy(h, s, n + 1);
  unsigned char o[6];
  sink += smlOBISFromString(h, o);
  free(h);
}

static Bytes readFile(const char *path)
{
  Bytes b;
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "cannot read %s\n", path);
    return b;
  }
  int c;
  while ((c = fgetc(f)) != EOF) b.push_back(c);
  fclose(f);
  return b;
}

static void writeFile(const char *path, const Bytes &b)
{
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fwrite(b.data(), 1, b.size(), f);
  fclose(f);
}

int main(int argc, char **argv)
{
  if (argc >= 2 && strcmp(argv[1], "--replay") == 0) {
    for (int i = 2; i < argc; i++) {
      rs = 1;
      runInput(readFile(argv[i]));
      printf("ok %s\n", argv[i]);
    }
    return 0;
  }
  if (argc < 4) {
    fprintf(stderr, "usage: fuzz <seconds> <seed> <input file> <seed dumps...>\n"
                    "       fuzz --replay <files...>\n");
    return 2;
  }
  int seconds = atoi(argv[1]);
  rs = strtoull(argv[2], 0, 10) * 2654435761ULL + 1;
  const char *current = argv[3];

  std::vector<Bytes> corpus;
  for (int i = 4; i < argc; i++) addFrames(corpus, readFile(argv[i]));
  for (size_t i = 0; i < corpus.size(); i++) {
    writeFile(current, corpus[i]);
    runInput(corpus[i]);
  }
  if (corpus.empty()) corpus.push_back(generate());

  time_t end = time(0) + seconds;
  unsigned long iterations = 0;
  while (time(0) < end) {
    for (int k = 0; k < 64; k++, iterations++) {
      Bytes b;
      unsigned mode = R(10);
      if (mode < 3)
        b = generate();
      else if (mode < 4) {
        b.resize(R(512));
        for (size_t i = 0; i < b.size(); i++) b[i] = rnd();
      }
      else {
        b = corpus[R(corpus.size())];
        mutate(b, corpus);
      }
      writeFile(current, b);
      runInput(b);
      fuzzFromString();
      /* generated messages are good starting points for mutation */
      if (mode < 3 && R(64) == 0 && corpus.size() < 2000) corpus.push_back(b);
    }
  }
  printf("%lu inputs, %lu with a valid checksum, %lu OBIS entries, %lu unit matches\n",
         iterations, finals, entries, unitHits);
  return 0;
}
