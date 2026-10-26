#include "ehz_bin.h"
#include "ehz_bin2.h"
#include "sml.h"
#include "unity.h"
#ifdef ARDUINO
#include "arduino.h"
#endif

// Tests two instances of SmlParser with slightly different server ids and
// input stream for the parser instances shifted by value of 'byteStreamShift' to check proper smlState operation.

class ExtractedData;

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)(SmlParser& smlParser, ExtractedData& extractedData);
} OBISHandler;

class ExtractedData
{
  public:
    ExtractedData()
    {
      manuf[0] = 0;
      serverIdLength = 0;
      t1Wh = -2;
      sumWh = -2;
      isFinal = false;
    }

    static const size_t MAX_STR_MANUF = 5;
    unsigned char manuf[MAX_STR_MANUF];
    static const size_t MAX_LEN_SERVERID = 20;
    int serverIdLength;
    unsigned char serverId[MAX_LEN_SERVERID];
    double t1Wh;
    double sumWh;
    bool isFinal;
};

// SmlParser1 instance already provided by sml parser library.
SmlParser SmlParser2;
ExtractedData extractedData1;
ExtractedData extractedData2;

void Manufacturer(SmlParser& smlParser, ExtractedData& extractedData) { smlParser.smlOBISManufacturer(extractedData.manuf, ExtractedData::MAX_STR_MANUF); }
void ServerId(SmlParser& smlParser, ExtractedData& extractedData) { extractedData.serverIdLength = smlParser.smlOBISServerId(extractedData.serverId, ExtractedData::MAX_LEN_SERVERID); }
void PowerT1(SmlParser& smlParser, ExtractedData& extractedData) { smlParser.smlOBISWh(extractedData.t1Wh); }
void PowerSum(SmlParser& smlParser, ExtractedData& extractedData) { smlParser.smlOBISWh(extractedData.sumWh); }

// clang-format off
OBISHandler OBISHandlers[] = {
  {{ 0x81, 0x81, 0xc7, 0x82, 0x03, 0xff }, &Manufacturer}, /* 129-129:199.130.3*255 */
  {{ 0x01, 0x00, 0x00, 0x00, 0x09, 0xff }, &ServerId},     /*   1-  0:  0.  0.9*255 (ServerId) */
  {{ 0x01, 0x00, 0x01, 0x08, 0x01, 0xff }, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
  {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};
// clang-format on

void setUp(void)
{
  unsigned int byteStreamShift = 20; 
  sml_states_t s1 = SML_START; 
  sml_states_t s2 = SML_START; 

  for (unsigned int i = 0; i < ehz_bin_len + byteStreamShift; ++i) {
    if (i < ehz_bin_len) {
        unsigned char c = ehz_bin[i];
        s1 = SmlParser1.smlState(c);
    }
    if (i >= byteStreamShift) {
        unsigned char c = ehz_bin2[i - byteStreamShift];
        s2 = SmlParser2.smlState(c);
    }
    
    if (s1 == SML_LISTEND) {
      /* check handlers on last received list */
      int iHandler;
      for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                         !(SmlParser1.smlOBISCheck(OBISHandlers[iHandler].OBIS));
           iHandler++)
        ;
      if (OBISHandlers[iHandler].Handler != 0) {
        OBISHandlers[iHandler].Handler(SmlParser1, extractedData1);
      }
    }
    if (s2 == SML_LISTEND) {
      /* check handlers on last received list */
      int iHandler;
      for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                         !(SmlParser2.smlOBISCheck(OBISHandlers[iHandler].OBIS));
           iHandler++)
        ;
      if (OBISHandlers[iHandler].Handler != 0) {
        OBISHandlers[iHandler].Handler(SmlParser2, extractedData2);
      }
    }

    if (s1 == SML_FINAL) {
      extractedData1.isFinal = true;
    }
    if (s2 == SML_FINAL) {
      extractedData2.isFinal = true;
    }    
  }
}

void tearDown(void) {
    // intentionally left blank, needed for native hosting in Windows (fixes unity.c undefined reference to 'tearDown')
}

void test_should_return_manufacturer(void)
{
  TEST_ASSERT_EQUAL_STRING("EMH", extractedData1.manuf);
  TEST_ASSERT_EQUAL_STRING("EMH", extractedData2.manuf);
}

void test_should_return_serverId(void)
{
  static const unsigned char expectedServerId1[]={ 0x06, 0x45, 0x4D, 0x48, 0x01, 0x00, 0x1D, 0x46, 0x15, 0xCA };
  TEST_ASSERT_EQUAL_MEMORY(expectedServerId1, extractedData1.serverId, extractedData1.serverIdLength);
  static const unsigned char expectedServerId2[]={ 0x06, 0x45, 0x4D, 0x48, 0x01, 0x00, 0x1D, 0x46, 0x16, 0xCA };
  TEST_ASSERT_EQUAL_MEMORY(expectedServerId2, extractedData2.serverId, extractedData2.serverIdLength);
}

void test_should_return_t1(void)
{ 
  TEST_ASSERT_EQUAL_DOUBLE(12345678.9, extractedData1.t1Wh);
  TEST_ASSERT_EQUAL_DOUBLE(12345678.9, extractedData2.t1Wh);
}

void test_should_return_SumWh(void)
{
  TEST_ASSERT_EQUAL_DOUBLE(7238000, extractedData1.sumWh);
  TEST_ASSERT_EQUAL_DOUBLE(7238000, extractedData2.sumWh);
}

void test_should_be_final(void)
{
  TEST_ASSERT_EQUAL_INT(1, extractedData1.isFinal);
  TEST_ASSERT_EQUAL_INT(1, extractedData2.isFinal);
}

// Mimics Serial.read(), which returns an int rvalue
static int readByte(unsigned int i) { return ehz_bin[i]; }

void test_should_accept_rvalue_byte(void)
{
  // smlState takes the byte by value, so smlState(Serial.read()) compiles (#16)
  SmlParser parser;
  sml_states_t s = SML_START;
  for (unsigned int i = 0; i < ehz_bin_len; ++i) {
    s = parser.smlState(readByte(i));
  }
  TEST_ASSERT_EQUAL_INT(SML_FINAL, s);
}

int runUnityTests(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_should_return_manufacturer);
  RUN_TEST(test_should_return_serverId);
  RUN_TEST(test_should_return_t1);
  RUN_TEST(test_should_return_SumWh);
  RUN_TEST(test_should_be_final);
  RUN_TEST(test_should_accept_rvalue_byte);
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
