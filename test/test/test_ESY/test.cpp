#include "data_bin.h"
#include "sml.h"
#include "unity.h"
#ifdef ARDUINO
#include "arduino.h"
#endif

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)();
} OBISHandler;

#define MAX_STR_MANUF 5
unsigned char manuf[MAX_STR_MANUF];
long int SumWh = -2;
bool isFinal = false;

void Manufacturer() { smlOBISManufacturer(manuf, MAX_STR_MANUF); }

void PowerSum()
{
  // Double on Arduino UNO is just 32 bit, so we must test against long int
  signed char sc = 0;
  smlOBISByUnit(SumWh, sc, SML_WATT_HOUR);
}

// clang-format off
OBISHandler OBISHandlers[] = {
  {{ 0x81, 0x81, 0xc7, 0x82, 0x03, 0xff }, &Manufacturer}, /* 129-129:199.130.3*255 */
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};
// clang-format on

void setUp(void)
{
  unsigned int i = 0;
  int iHandler = 0;
  unsigned char c;
  sml_states_t s;
  manuf[0] = 0;

  for (i = 0; i < data_bin_len; ++i) {
#ifdef ARDUINO
    c = pgm_read_word_near(data_bin + i);
#else
    c = data_bin[i];
#endif

    s = smlState(c);
    if (s == SML_LISTEND) {
      /* check handlers on last received list */
      for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                         !(smlOBISCheck(OBISHandlers[iHandler].OBIS));
           iHandler++)
        ;
      if (OBISHandlers[iHandler].Handler != 0) {
        OBISHandlers[iHandler].Handler();
      }
    }
    if (s == SML_FINAL) {
      isFinal = true;
    }
  }
}

void test_should_return_manufacturer(void)
{
  TEST_ASSERT_EQUAL_STRING("ESY", manuf);
}

void test_should_return_SumWh(void)
{
  TEST_ASSERT_EQUAL_INT(29416471626, SumWh);
}

void test_should_be_final(void) { TEST_ASSERT_EQUAL_INT(1, isFinal); }

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_return_manufacturer);
  //  RUN_TEST(test_should_return_t1);
  RUN_TEST(test_should_return_SumWh);
  RUN_TEST(test_should_be_final);
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
