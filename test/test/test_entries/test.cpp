#include "../test_EMH/ehz_bin.h"
#include "sml.h"
#include "unity.h"
#include <string.h>
#ifdef ARDUINO
#include "arduino.h"
#endif

// Reads all entries via smlOBISEntry without knowing the OBIS codes in advance.

#define MAX_ENTRIES 7
#define MAX_STR_OBIS 24
sml_obis_entry_t entries[MAX_ENTRIES];
int entryCount = 0;

void setUp(void)
{
  unsigned int i = 0;
  unsigned char c;
  sml_states_t s;
  sml_obis_entry_t entry;
  entryCount = 0;

  for (i = 0; i < ehz_bin_len; ++i) {
    c = ehz_bin[i];
    s = smlState(c);
    if (s == SML_LISTEND && smlOBISEntry(entry)) {
      if (entryCount < MAX_ENTRIES) {
        entries[entryCount] = entry;
      }
      entryCount++;
    }
  }
}

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

void assertOBIS(const char *expected, int index)
{
  char obis[MAX_STR_OBIS];
  smlOBISToString(entries[index].obis, obis, MAX_STR_OBIS);
  TEST_ASSERT_EQUAL_STRING(expected, obis);
}

void test_should_find_all_entries(void)
{
  TEST_ASSERT_EQUAL_INT(7, entryCount);
  assertOBIS("129-129:199.130.3*255", 0);
  assertOBIS("1-0:0.0.9*255", 1);
  assertOBIS("1-0:1.8.0*255", 2);
  assertOBIS("1-0:1.8.1*255", 3);
  assertOBIS("1-0:1.8.2*255", 4);
  assertOBIS("1-0:15.7.0*255", 5);
  assertOBIS("129-129:199.130.5*255", 6);
}

void test_should_return_numeric_values(void)
{
  TEST_ASSERT_EQUAL_INT(SML_DATA_SIGNED_INT, entries[3].type);
  TEST_ASSERT_EQUAL_INT(SML_WATT_HOUR, entries[3].unit);
  TEST_ASSERT_EQUAL_INT(-1, entries[3].scaler);
  TEST_ASSERT_TRUE(entries[3].value == 123456789LL);

  TEST_ASSERT_EQUAL_INT(SML_WATT, entries[5].unit);
  TEST_ASSERT_EQUAL_INT(-1, entries[5].scaler);
  TEST_ASSERT_TRUE(entries[5].value == 12133LL);
}

void test_should_return_octet_strings(void)
{
  // raw bytes point into the parser buffer, so only check the last entry
  TEST_ASSERT_EQUAL_INT(SML_DATA, entries[6].type);
  TEST_ASSERT_EQUAL_INT(48, entries[6].length);
  TEST_ASSERT_EQUAL_INT(SML_DATA_OCTET_STRING, entries[0].type);
  TEST_ASSERT_EQUAL_INT(3, entries[0].length);
}

void test_should_parse_obis_string(void)
{
  unsigned char code[6];
  static const unsigned char expected[] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};
  static const unsigned char manufacturer[] = {0x81, 0x81, 0xc7, 0x82, 0x03, 0xff};

  TEST_ASSERT_TRUE(smlOBISFromString("1-0:1.8.0*255", code));
  TEST_ASSERT_EQUAL_MEMORY(expected, code, 6);
  memset(code, 0, sizeof(code));
  TEST_ASSERT_TRUE(smlOBISFromString("1-0:1.8.0", code));
  TEST_ASSERT_EQUAL_MEMORY(expected, code, 6);
  TEST_ASSERT_TRUE(smlOBISFromString("129-129:199.130.3*255", code));
  TEST_ASSERT_EQUAL_MEMORY(manufacturer, code, 6);

  TEST_ASSERT_FALSE(smlOBISFromString("", code));
  TEST_ASSERT_FALSE(smlOBISFromString("1-0:1.8", code));
  TEST_ASSERT_FALSE(smlOBISFromString("1-0:1.8.0*256", code));
  TEST_ASSERT_FALSE(smlOBISFromString("1.0.1.8.0.255", code));
  TEST_ASSERT_FALSE(smlOBISFromString("1-0:1.8.0*255x", code));
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_find_all_entries);
  RUN_TEST(test_should_return_numeric_values);
  RUN_TEST(test_should_return_octet_strings);
  RUN_TEST(test_should_parse_obis_string);
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
