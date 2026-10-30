#include "sml.h"
#include "unity.h"
#ifdef ARDUINO
#include "arduino.h"
#endif

// Issues found by test/fuzz. The native environment is built with
// -fsanitize=undefined, so undefined behaviour aborts these tests.

void setUp(void) {}

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

static void feed(SmlParser &parser, const unsigned char *data, int len)
{
  for (int i = 0; i < len; i++)
    parser.smlState(data[i]);
}

static void feedStart(SmlParser &parser)
{
  static const unsigned char start[] = {0x1b, 0x1b, 0x1b, 0x1b,
                                        0x01, 0x01, 0x01, 0x01};
  feed(parser, start, sizeof(start));
}

/* A list of n unsigned 8 bit values, the last one must end the list */
static void assertListOf(SmlParser &parser, int n)
{
  for (int i = 0; i < n; i++) {
    TEST_ASSERT_EQUAL_INT(SML_DATA_UNSIGNED_INT, parser.smlState(0x62));
    TEST_ASSERT_EQUAL_INT(i == n - 1 ? SML_LISTEND : SML_DATAEND,
                          parser.smlState((unsigned char)i));
  }
}

void test_should_not_read_past_the_list_buffer(void)
{
  /* fills the 80 byte list buffer exactly up to the size of entry 6 */
  SmlParser parser;
  unsigned char bytes[10];
  unsigned char manuf[10];
  feedStart(parser);
  parser.smlState(0x77);
  for (int e = 0; e < 5; e++) {
    int size = e < 4 ? 14 : 13;
    parser.smlState(size + 1);
    for (int i = 0; i < size; i++)
      parser.smlState(0x30 + i);
  }
  parser.smlState(0x01);
  TEST_ASSERT_EQUAL_INT(SML_LISTEND, parser.smlState(0x01));

  TEST_ASSERT_EQUAL_INT(0, parser.smlOBISServerId(bytes, sizeof(bytes)));
  parser.smlOBISManufacturer(manuf, sizeof(manuf));
  TEST_ASSERT_EQUAL_UINT8(0, manuf[0]);
}

void test_should_reset_scaler_if_unit_does_not_match(void)
{
  static const unsigned char wh[] = {
      0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xff, 0x01, 0x01,
      0x62, 0x1e,       /* unit Wh */
      0x52, 0xff,       /* scaler -1 */
      0x62, 0x64, 0x01}; /* 100 */
  static const unsigned char w[] = {
      0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xff, 0x01, 0x01,
      0x62, 0x1b,       /* unit W */
      0x52, 0xff,       /* scaler -1 */
      0x62, 0x64, 0x01};
  SmlParser parser;
  double value;

  feedStart(parser);
  feed(parser, wh, sizeof(wh));
  parser.smlOBISWh(value);
  TEST_ASSERT_EQUAL_DOUBLE(10, value);

  /* the scaler of the previous call must not be applied to -1 */
  feed(parser, w, sizeof(w));
  parser.smlOBISWh(value);
  TEST_ASSERT_EQUAL_DOUBLE(-1, value);
}

void test_should_parse_extended_list_with_16_entries(void)
{
  SmlParser parser;
  feedStart(parser);
  parser.smlState(0xf1);
  TEST_ASSERT_EQUAL_INT(SML_LISTSTART, parser.smlState(0x00)); /* 0x10 entries */
  assertListOf(parser, 16);
}

void test_should_parse_extended_list_with_18_entries(void)
{
  SmlParser parser;
  feedStart(parser);
  parser.smlState(0xf1);
  TEST_ASSERT_EQUAL_INT(SML_LISTSTART, parser.smlState(0x02)); /* 0x12 entries */
  assertListOf(parser, 18);
}

void test_should_parse_list_with_more_than_127_entries(void)
{
  SmlParser parser;
  feedStart(parser);
  parser.smlState(0xfc);
  TEST_ASSERT_EQUAL_INT(SML_LISTSTART, parser.smlState(0x08)); /* 0xc8 entries */
  assertListOf(parser, 200);
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_not_read_past_the_list_buffer);
  RUN_TEST(test_should_reset_scaler_if_unit_does_not_match);
  RUN_TEST(test_should_parse_extended_list_with_16_entries);
  RUN_TEST(test_should_parse_extended_list_with_18_entries);
  RUN_TEST(test_should_parse_list_with_more_than_127_entries);
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
