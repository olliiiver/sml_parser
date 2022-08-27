#include "Arduino.h"
#include <SPI.h>
#include <U8g2lib.h>
#include <hal/hal.h>
#include <lmic.h>

#include "keys.h"
#include "sml.h"
#include <SoftwareSerial.h>
#include <arduino_lmic_hal_boards.h>

#define rxPin 36

SoftwareSerial inputSerial;
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/15, /* data=*/4,
                                       /* reset=*/16);

sml_states_t currentState;
unsigned char currentChar = 0;
unsigned long counter = 0, transmitted = 0;
char buffer[50];
char statusMsg[30] = "Unknown";
char floatBuffer[20];
double T1Wh = -2, SumWh = -2, T1WhSend = -2, SumWhSend = -2;

static osjob_t sendjob;

// Schedule TX every this many seconds
const unsigned TX_INTERVAL = 500;

void os_getArtEui(u1_t *buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui(u1_t *buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey(u1_t *buf) { memcpy_P(buf, APPKEY, 16); }

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 34, 35},
};

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)();
} OBISHandler;

void PowerT1() { smlOBISWh(T1Wh); }

void PowerSum() { smlOBISWh(SumWh); }

// clang-format off
OBISHandler OBISHandlers[] = {
    {{0x01, 0x00, 0x01, 0x08, 0x01, 0xff}, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
    {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
    {{0}, 0}
};
// clang-format on

void printHex2(unsigned v)
{
  v &= 0xff;
  if (v < 16)
    Serial.print('0');
  Serial.print(v, HEX);
}

void updateDisplay()
{
  u8x8.drawString(0, 2, statusMsg);

  sprintf(buffer, "Msg: %lu", counter);
  u8x8.drawString(0, 3, buffer);

  dtostrf(T1WhSend, 10, 1, floatBuffer);
  sprintf(buffer, "T1.: %s", floatBuffer);
  u8x8.drawString(0, 4, buffer);

  dtostrf(SumWhSend, 10, 1, floatBuffer);
  sprintf(buffer, "Sum: %s", floatBuffer);
  u8x8.drawString(0, 5, buffer);

  sprintf(buffer, "Tns: %lu", transmitted);
  u8x8.drawString(0, 6, buffer);
}

void do_send(osjob_t *j)
{
  unsigned char floatBuffer[20];
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else {
    // @todo should be optimized
    dtostrf(SumWhSend, 10, 1, (char *)floatBuffer);
    LMIC_setTxData2(1, floatBuffer, strlen((char *)floatBuffer), 0);
    Serial.printf("---> Packet queued %s", floatBuffer);
    Serial.println("");
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent(ev_t ev)
{
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED"));
    {
      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
      Serial.print("netid: ");
      Serial.println(netid, DEC);
      Serial.print("devaddr: ");
      Serial.println(devaddr, HEX);
      Serial.print("AppSKey: ");
      for (size_t i = 0; i < sizeof(artKey); ++i) {
        if (i != 0)
          Serial.print("-");
        printHex2(artKey[i]);
      }
      Serial.println("");
      Serial.print("NwkSKey: ");
      for (size_t i = 0; i < sizeof(nwkKey); ++i) {
        if (i != 0)
          Serial.print("-");
        printHex2(nwkKey[i]);
      }
      Serial.println();
      sprintf(statusMsg, "Joined");
      updateDisplay();
    }
    // Disable link check validation (automatically enabled
    // during join, but because slow data rates change max TX
    // size, we don't use it in this example.
    LMIC_setLinkCheckMode(0);
    break;
  /*
  || This event is defined but not used in the code. No
  || point in wasting codespace on it.
  ||
  || case EV_RFU1:
  ||     Serial.println(F("EV_RFU1"));
  ||     break;
  */
  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    sprintf(statusMsg, "Join failed");
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    sprintf(statusMsg, "rejoin failed");
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("---> EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack"));
    if (LMIC.dataLen) {
      Serial.print(F("Received "));
      Serial.print(LMIC.dataLen);
      Serial.println(F(" bytes of payload"));
    }
    // Schedule next transmission
    os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL),
                        do_send);
    transmitted++;
    sprintf(statusMsg, "TX Compl");
    updateDisplay();
    break;
  case EV_LOST_TSYNC:
    sprintf(statusMsg, "Lost sync");
    updateDisplay();
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    sprintf(statusMsg, "Reset sync");
    updateDisplay();
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    sprintf(statusMsg, "Link dead");
    updateDisplay();
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    sprintf(statusMsg, "Link alive");
    updateDisplay();
    Serial.println(F("EV_LINK_ALIVE"));
    break;
  /*
  || This event is defined but not used in the code. No
  || point in wasting codespace on it.
  ||
  || case EV_SCAN_FOUND:
  ||    Serial.println(F("EV_SCAN_FOUND"));
  ||    break;
  */
  case EV_TXSTART:
    sprintf(statusMsg, "tx start");
    updateDisplay();
    Serial.println(F("EV_TXSTART"));
    break;
  case EV_TXCANCELED:
    Serial.println(F("EV_TXCANCELED"));
    break;
  case EV_RXSTART:
    /* do not print anything -- it wrecks timing */
    break;
  case EV_JOIN_TXCOMPLETE:
    Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
    break;

  default:
    Serial.print(F("Unknown event: "));
    Serial.println((unsigned)ev);
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "SML to LoraWAN");
  u8x8.drawString(0, 2, "Waiting");

  // LMIC init
  os_init();

  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  pinMode(rxPin, INPUT);

  inputSerial.begin(9600, SWSERIAL_8N1, rxPin, -1, false, 0, 95);
  inputSerial.enableRx(true);
  inputSerial.enableTx(false);

  do_send(&sendjob);
}

void readByte()
{
  unsigned int iHandler = 0;
  currentState = smlState(currentChar);
  if (currentState == SML_START) {
    /* reset local vars */
    T1Wh = -3;
    SumWh = -3;
  }
  if (currentState == SML_LISTEND) {
    /* check handlers on last received list */
    for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                       !(smlOBISCheck(OBISHandlers[iHandler].OBIS));
         iHandler++)
      ;
    if (OBISHandlers[iHandler].Handler != 0) {
      OBISHandlers[iHandler].Handler();
    }
  }
  if (currentState == SML_UNEXPECTED) {
    Serial.print(F(">>> Unexpected byte!\n"));
  }
  if (currentState == SML_FINAL) {
    SumWhSend = SumWh;
    T1WhSend = T1Wh;
    updateDisplay();
    counter++;
    Serial.print(F(">>> Successfully received a complete message!\n"));
  }
}

void loop()
{
  os_runloop_once();
  if (inputSerial.available()) {
    currentChar = inputSerial.read();
    readByte();
  }
}
