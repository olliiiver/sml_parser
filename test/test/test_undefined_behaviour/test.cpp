#include "sml.h"
#include "unity.h"
#ifdef ARDUINO
#include "arduino.h"
#endif

// Negative signed values used to be decoded by left-shifting a negative
// long long, which is undefined behaviour before C++20. The native test
// environment is built with -fsanitize=undefined, so a regression aborts
// these tests instead of silently relying on the compiler.

static const unsigned char obis180[] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};

void setUp(void) {}

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

/* Feed a single OBIS list entry (1-0:1.8.0*255, Wh, scaler 0) with the given
   value (type-length byte followed by the data) and return the last state. */
sml_states_t parseEntry(SmlParser &parser, const unsigned char *value, int len)
{
  static const unsigned char start[] = {0x1b, 0x1b, 0x1b, 0x1b,
                                        0x01, 0x01, 0x01, 0x01};
  static const unsigned char head[] = {
      0x77,                                     /* list with 7 entries */
      0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xff, /* OBIS */
      0x01,                                     /* status */
      0x01,                                     /* valTime */
      0x62, 0x1e,                               /* unit: Wh */
      0x52, 0x00};                              /* scaler: 0 */
  sml_states_t s = SML_START;
  unsigned int i;
  for (i = 0; i < sizeof(start); i++)
    parser.smlState(start[i]);
  for (i = 0; i < sizeof(head); i++)
    parser.smlState(head[i]);
  for (i = 0; i < (unsigned int)len; i++)
    parser.smlState(value[i]);
  s = parser.smlState(0x01); /* valueSignature */
  return s;
}

void assertValue(long long expected, const unsigned char *value, int len)
{
  SmlParser parser;
  long long val = 0;
  signed char scaler = 1;
  sml_obis_entry_t entry;

  TEST_ASSERT_EQUAL_INT(SML_LISTEND, parseEntry(parser, value, len));
  TEST_ASSERT_TRUE(parser.smlOBISCheck(obis180));

  parser.smlOBISByUnit(val, scaler, SML_WATT_HOUR);
  TEST_ASSERT_TRUE(val == expected);

  TEST_ASSERT_TRUE(parser.smlOBISEntry(entry));
  TEST_ASSERT_EQUAL_INT(SML_DATA_SIGNED_INT, entry.type);
  TEST_ASSERT_TRUE(entry.value == expected);
}

void test_should_decode_negative_16bit(void)
{
  static const unsigned char value[] = {0x53, 0xfe, 0xfb};
  assertValue(-261, value, sizeof(value));
}

void test_should_decode_minus_one_64bit(void)
{
  static const unsigned char value[] = {0x59, 0xff, 0xff, 0xff, 0xff,
                                        0xff, 0xff, 0xff, 0xff};
  assertValue(-1, value, sizeof(value));
}

void test_should_decode_min_64bit(void)
{
  static const unsigned char value[] = {0x59, 0x80, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00};
  assertValue(-9223372036854775807LL - 1, value, sizeof(value));
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_decode_negative_16bit);
  RUN_TEST(test_should_decode_minus_one_64bit);
  RUN_TEST(test_should_decode_min_64bit);
  return UNITY_END();
}

/**
 * For native dev-platform or for some embedded frameworks
 */
int main(void) { return runUnityTests(); }

/**
 * For Arduino framework
 */
void setup()
{
// Wait ~2 seconds before the Unity test runner
// establishes connection with a board Serial interface
#ifdef ARDUINO
  delay(2000);
#endif
  runUnityTests();
}
void loop() {}

/**
 * For ESP-IDF framework
 */
void app_main() { runUnityTests(); }
