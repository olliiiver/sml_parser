#include "sml.h"
#include "unity.h"
#ifdef ARDUINO
#include "arduino.h"
#endif

// Lists nested deeper than the parser's tree size must not write past the
// internal node array (issue #24). Before the fix, the out of bounds write
// clobbered the current tree level and the parser rejected valid data.

void setUp(void) {}

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

void test_should_parse_data_after_deeply_nested_lists(void)
{
  SmlParser parser;
  unsigned char start[] = {0x1b, 0x1b, 0x1b, 0x1b, 0x01, 0x01, 0x01, 0x01};
  for (unsigned int i = 0; i < sizeof(start); i++) {
    parser.smlState(start[i]);
  }
  /* 12 nested lists with 15 entries each, more than the parser can track */
  for (int i = 0; i < 12; i++) {
    unsigned char c = 0x7f;
    TEST_ASSERT_EQUAL_INT(SML_LISTSTART, parser.smlState(c));
  }
  /* two unsigned 8 bit values inside the innermost list */
  for (int i = 0; i < 2; i++) {
    unsigned char tl = 0x62;
    unsigned char value = 0x05;
    TEST_ASSERT_EQUAL_INT(SML_DATA_UNSIGNED_INT, parser.smlState(tl));
    TEST_ASSERT_EQUAL_INT(SML_DATAEND, parser.smlState(value));
  }
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_parse_data_after_deeply_nested_lists);
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
