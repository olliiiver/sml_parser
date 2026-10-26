#include "ehz_bin.h"
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
double T1Wh = -2, SumWh = -2;
#define MAX_LEN_DEVICEID 10
int deviceIdLength = 0;
unsigned char deviceId[MAX_LEN_DEVICEID];
bool isFinal = false;

void PowerSum() { smlOBISWh(SumWh); }
void DeviceId() { deviceIdLength = smlOBISServerId(deviceId, MAX_LEN_DEVICEID); }

// clang-format off
OBISHandler OBISHandlers[] = {
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0x01, 0x00, 0x60, 0x01, 0x00, 0xff }, &DeviceId},     /*   1-  0: 96.  1.0*255 (device id) */
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

  for (i = 0; i < ehz_bin_len; ++i) {
    c = ehz_bin[i];
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

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

void test_should_return_SumWh(void)
{
  TEST_ASSERT_EQUAL_DOUBLE(1608652.2, SumWh);
}

void test_should_return_DeviceId(void)
{
  static const unsigned char expectedDeviceId[] = { 0x0A, 0x01, 0x45, 0x46, 0x52, 0x21, 0x01, 0xE8, 0xB2, 0xF0 };
  TEST_ASSERT_EQUAL_INT(sizeof(expectedDeviceId), deviceIdLength);
  TEST_ASSERT_EQUAL_MEMORY(expectedDeviceId, deviceId, deviceIdLength);
}

void test_should_be_final(void) { TEST_ASSERT_EQUAL_INT(1, isFinal); }

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_return_SumWh);
  RUN_TEST(test_should_return_DeviceId);
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
